#include "ESPConfig.h"

#include <cstdio>
#include <cstdlib>

namespace ESPConfig {

void Manager::addInteger(const String& key, const String& label, int defaultValue, bool required,
                         std::function<bool(const String&)> validator) {
    ConfigField field;
    field.key = key;
    field.label = label;
    field.type = FieldType::Integer;
    field.required = required;
    field.intValue = defaultValue;
    field.value = String(defaultValue);
    field.validator = validator;
    _fields.push_back(field);
}

void Manager::addString(const String& key, const String& label, const String& defaultValue, bool required,
                        std::function<bool(const String&)> validator) {
    ConfigField field;
    field.key = key;
    field.label = label;
    field.type = FieldType::String;
    field.required = required;
    field.value = defaultValue;
    field.validator = validator;
    _fields.push_back(field);
}

void Manager::addBoolean(const String& key, const String& label, bool defaultValue, bool required,
                         std::function<bool(const String&)> validator) {
    ConfigField field;
    field.key = key;
    field.label = label;
    field.type = FieldType::Boolean;
    field.required = required;
    field.boolValue = defaultValue;
    field.value = defaultValue ? "true" : "false";
    field.validator = validator;
    _fields.push_back(field);
}

void Manager::addArray(const String& key, const String& label, ValueType itemType, bool required) {
    ConfigField field;
    field.key = key;
    field.label = label;
    field.type = FieldType::Array;
    field.required = required;
    field.itemType = itemType;
    _fields.push_back(field);
}

void Manager::addPairArray(const String& key, const String& label, bool required) {
    addPairArray(key, label, ValueType::String, ValueType::String, required);
}

void Manager::addPairArray(const String& key, const String& label, ValueType firstType,
                           ValueType secondType, bool required) {
    ConfigField field;
    field.key = key;
    field.label = label;
    field.type = FieldType::PairArray;
    field.required = required;
    field.firstPairType = firstType;
    field.secondPairType = secondType;
    _fields.push_back(field);
}

void Manager::setOnChange(const String& key, ChangeHandler handler) {
    ConfigField* field = fieldByKey(key);
    if (field) {
        field->onChange = handler;
    }
}

void Manager::setOnChange(const String& key, std::function<void()> handler) {
    if (!handler) {
        setOnChange(key, ChangeHandler());
        return;
    }
    setOnChange(key, [handler](const ConfigField&) {
        handler();
    });
}

void Manager::addAction(const String& key, const String& label, std::function<void()> handler,
                        bool requiresConfirmation) {
    ConfigAction action;
    action.key = key;
    action.label = label;
    action.requiresConfirmation = requiresConfirmation;
    action.handler = handler;
    _actions.push_back(action);
}

bool Manager::setFieldValue(const String& key, const String& value) {
    ConfigField* field = fieldByKey(key);
    if (!field || field->type == FieldType::Array || field->type == FieldType::PairArray ||
        (field->required && value.length() == 0)) {
        return false;
    }
    if (field->validator && !field->validator(value)) {
        return false;
    }

    switch (field->type) {
        case FieldType::Integer: {
            char* end = nullptr;
            const long parsed = std::strtol(value.c_str(), &end, 10);
            if (value.length() == 0 || end == value.c_str() || *end != '\0') {
                return false;
            }
            field->intValue = static_cast<int>(parsed);
            field->value = String(field->intValue);
            break;
        }
        case FieldType::String:
            field->value = value;
            break;
        case FieldType::Boolean:
            if (value.equalsIgnoreCase("true") || value == "1") {
                field->boolValue = true;
                field->value = "true";
            } else if (value.equalsIgnoreCase("false") || value == "0") {
                field->boolValue = false;
                field->value = "false";
            } else {
                return false;
            }
            break;
        case FieldType::Array:
        case FieldType::PairArray:
            return false;
    }

    if (field->onChange) {
        field->onChange(*field);
    }
    return true;
}

bool Manager::addItem(const String& key, const String& value) {
    ConfigField* field = fieldByKey(key);
    if (!field || field->type != FieldType::Array) {
        return false;
    }
    String normalized;
    if (!normalizeValue(field->itemType, value, normalized)) {
        return false;
    }
    field->items.push_back(normalized);
    if (field->onChange) {
        field->onChange(*field);
    }
    return true;
}

bool Manager::clearItems(const String& key) {
    ConfigField* field = fieldByKey(key);
    if (!field || field->type != FieldType::Array) {
        return false;
    }
    field->items.clear();
    if (field->onChange) {
        field->onChange(*field);
    }
    return true;
}

bool Manager::addPair(const String& key, const String& first, const String& second) {
    ConfigField* field = fieldByKey(key);
    if (!field || field->type != FieldType::PairArray) {
        return false;
    }
    String normalizedFirst;
    String normalizedSecond;
    if (!normalizeValue(field->firstPairType, first, normalizedFirst) ||
        !normalizeValue(field->secondPairType, second, normalizedSecond)) {
        return false;
    }
    field->pairs.emplace_back(normalizedFirst, normalizedSecond);
    if (field->onChange) {
        field->onChange(*field);
    }
    return true;
}

bool Manager::clearPairs(const String& key) {
    ConfigField* field = fieldByKey(key);
    if (!field || field->type != FieldType::PairArray) {
        return false;
    }
    field->pairs.clear();
    if (field->onChange) {
        field->onChange(*field);
    }
    return true;
}

bool Manager::hasAction(const String& key) const {
    for (const ConfigAction& action : _actions) {
        if (action.key == key && action.handler) {
            return true;
        }
    }
    return false;
}

bool Manager::invokeAction(const String& key) {
    for (const ConfigAction& action : _actions) {
        if (action.key == key && action.handler) {
            action.handler();
            return true;
        }
    }
    return false;
}

String Manager::toJson() const {
    String json = "{";
    for (size_t i = 0; i < _fields.size(); ++i) {
        const ConfigField& field = _fields[i];
        json += "\"" + escapeJson(field.key) + "\":";
        switch (field.type) {
            case FieldType::Integer:
                json += String(field.intValue);
                break;
            case FieldType::String:
                json += "\"" + escapeJson(field.value) + "\"";
                break;
            case FieldType::Boolean:
                json += field.boolValue ? "true" : "false";
                break;
            case FieldType::Array:
                json += "[";
                for (size_t j = 0; j < field.items.size(); ++j) {
                    if (j > 0) {
                        json += ",";
                    }
                    json += valueJson(field.itemType, field.items[j]);
                }
                json += "]";
                break;
            case FieldType::PairArray:
                json += "[";
                for (size_t j = 0; j < field.pairs.size(); ++j) {
                    if (j > 0) {
                        json += ",";
                    }
                    json += "{\"first\":" + valueJson(field.firstPairType, field.pairs[j].first) +
                            ",\"second\":" + valueJson(field.secondPairType, field.pairs[j].second) + "}";
                }
                json += "]";
                break;
        }
        if (i + 1 < _fields.size()) {
            json += ",";
        }
    }
    json += "}";
    return json;
}

String Manager::schemaJson() const {
    String json = "[";
    for (size_t i = 0; i < _fields.size(); ++i) {
        const ConfigField& field = _fields[i];
        json += "{\"key\":\"" + escapeJson(field.key) + "\",";
        json += "\"label\":\"" + escapeJson(field.label) + "\",";
        json += "\"type\":\"" + fieldTypeName(field.type) + "\",";
        json += "\"required\":";
        json += field.required ? "true" : "false";
        if (field.type == FieldType::Array) {
            json += ",\"itemType\":\"" + valueTypeName(field.itemType) + "\"";
        } else if (field.type == FieldType::PairArray) {
            json += ",\"firstType\":\"" + valueTypeName(field.firstPairType) + "\"";
            json += ",\"secondType\":\"" + valueTypeName(field.secondPairType) + "\"";
        }
        json += "}";
        if (i + 1 < _fields.size()) {
            json += ",";
        }
    }
    json += "]";
    return json;
}

String Manager::actionsJson() const {
    String json = "[";
    for (size_t i = 0; i < _actions.size(); ++i) {
        const ConfigAction& action = _actions[i];
        json += "{\"key\":\"" + escapeJson(action.key) + "\",";
        json += "\"label\":\"" + escapeJson(action.label) + "\",";
        json += "\"requiresConfirmation\":";
        json += action.requiresConfirmation ? "true" : "false";
        json += "}";
        if (i + 1 < _actions.size()) {
            json += ",";
        }
    }
    json += "]";
    return json;
}

const ConfigField* Manager::getField(const String& key) const {
    return fieldByKey(key);
}

const std::vector<ConfigField>& Manager::getFields() const {
    return _fields;
}

ConfigField* Manager::fieldByKey(const String& key) {
    for (ConfigField& field : _fields) {
        if (field.key == key) {
            return &field;
        }
    }
    return nullptr;
}

const ConfigField* Manager::fieldByKey(const String& key) const {
    for (const ConfigField& field : _fields) {
        if (field.key == key) {
            return &field;
        }
    }
    return nullptr;
}

String Manager::escapeJson(const String& value) const {
    String escaped;
    for (size_t i = 0; i < value.length(); ++i) {
        const char c = value[i];
        switch (c) {
            case '\\': escaped += "\\\\"; break;
            case '"': escaped += "\\\""; break;
            case '\b': escaped += "\\b"; break;
            case '\f': escaped += "\\f"; break;
            case '\n': escaped += "\\n"; break;
            case '\r': escaped += "\\r"; break;
            case '\t': escaped += "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    char sequence[7];
                    std::snprintf(sequence, sizeof(sequence), "\\u%04x", static_cast<unsigned char>(c));
                    escaped += sequence;
                } else {
                    escaped += c;
                }
                break;
        }
    }
    return escaped;
}

String Manager::fieldTypeName(FieldType type) const {
    switch (type) {
        case FieldType::Integer: return "integer";
        case FieldType::String: return "string";
        case FieldType::Boolean: return "boolean";
        case FieldType::Array: return "array";
        case FieldType::PairArray: return "pair_array";
    }
    return "unknown";
}

String Manager::valueTypeName(ValueType type) const {
    switch (type) {
        case ValueType::String: return "string";
        case ValueType::Integer: return "integer";
        case ValueType::Boolean: return "boolean";
    }
    return "unknown";
}

bool Manager::normalizeValue(ValueType type, const String& input, String& output) const {
    switch (type) {
        case ValueType::String:
            output = input;
            return true;
        case ValueType::Integer: {
            char* end = nullptr;
            const long parsed = std::strtol(input.c_str(), &end, 10);
            if (input.length() == 0 || end == input.c_str() || *end != '\0') {
                return false;
            }
            output = String(parsed);
            return true;
        }
        case ValueType::Boolean:
            if (input.equalsIgnoreCase("true") || input == "1") {
                output = "true";
                return true;
            }
            if (input.equalsIgnoreCase("false") || input == "0") {
                output = "false";
                return true;
            }
            return false;
    }
    return false;
}

String Manager::valueJson(ValueType type, const String& value) const {
    if (type == ValueType::String) {
        return "\"" + escapeJson(value) + "\"";
    }
    return value;
}

} // namespace ESPConfig
