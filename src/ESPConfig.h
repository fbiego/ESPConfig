#pragma once

#include <Arduino.h>
#include <functional>
#include <vector>

#include "ESPConfigVersion.h"

namespace ESPConfig {

enum class FieldType {
    Integer,
    String,
    Boolean,
    Array,
    PairArray
};

enum class ValueType {
    String,
    Integer,
    Boolean
};

enum class ControlType {
    Default,
    Slider,
    Switch,
    Color
};

using PairValueType = ValueType;

struct ConfigField;
using ChangeHandler = std::function<void(const ConfigField&)>;

struct ConfigField {
    String key;
    String label;
    FieldType type;
    bool required = false;
    String value;
    int intValue = 0;
    bool boolValue = false;
    std::vector<String> items;
    ValueType itemType = ValueType::String;
    std::vector<std::pair<String, String>> pairs;
    ValueType firstPairType = ValueType::String;
    ValueType secondPairType = ValueType::String;
    ControlType control = ControlType::Default;
    int minimum = 0;
    int maximum = 100;
    int step = 1;
    bool immediateUpdate = false;
    std::function<bool(const String&)> validator;
    ChangeHandler onChange;
};

struct ConfigAction {
    String key;
    String label;
    bool requiresConfirmation = false;
    std::function<void()> handler;
};

class ESPConfigManager {
public:
    void addInteger(const String& key, const String& label, int defaultValue = 0, bool required = false,
                    std::function<bool(const String&)> validator = nullptr);
    void addString(const String& key, const String& label, const String& defaultValue = "", bool required = false,
                   std::function<bool(const String&)> validator = nullptr);
    void addBoolean(const String& key, const String& label, bool defaultValue = false, bool required = false,
                    std::function<bool(const String&)> validator = nullptr);
    void addArray(const String& key, const String& label, ValueType itemType = ValueType::String,
                  bool required = false);
    void addPairArray(const String& key, const String& label, bool required = false);
    void addPairArray(const String& key, const String& label, ValueType firstType, ValueType secondType,
                      bool required = false);

    void setOnChange(const String& key, ChangeHandler handler);
    void setOnChange(const String& key, std::function<void()> handler);
    void setOnChange(ChangeHandler handler);
    void setOnChange(std::function<void()> handler);
    bool setSlider(const String& key, int minimum, int maximum, int step = 1);
    bool setSwitch(const String& key);
    bool setColorPicker(const String& key);
    bool setImmediateUpdate(const String& key, bool enabled = true);
    void addAction(const String& key, const String& label, std::function<void()> handler,
                   bool requiresConfirmation = false);

    bool setFieldValue(const String& key, const String& value);
    bool addItem(const String& key, const String& value);
    bool clearItems(const String& key);
    bool addPair(const String& key, const String& first, const String& second);
    bool clearPairs(const String& key);
    bool hasAction(const String& key) const;
    bool invokeAction(const String& key);

    String toJson() const;
    String schemaJson() const;
    String actionsJson() const;
    const ConfigField* getField(const String& key) const;
    const std::vector<ConfigField>& getFields() const;

private:
    std::vector<ConfigField> _fields;
    std::vector<ConfigAction> _actions;
    ChangeHandler _onChange;

    ConfigField* fieldByKey(const String& key);
    const ConfigField* fieldByKey(const String& key) const;
    void notifyChanged(const ConfigField& field);
    String fieldTypeName(FieldType type) const;
    String valueTypeName(ValueType type) const;
    String controlTypeName(ControlType type) const;
    bool normalizeValue(ValueType type, const String& input, String& output) const;
    String valueJson(ValueType type, const String& value) const;
    String escapeJson(const String& value) const;
};

} // namespace ESPConfig
