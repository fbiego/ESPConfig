#pragma once

#define ESPCONFIG_VERSION_MAJOR 1
#define ESPCONFIG_VERSION_MINOR 1
#define ESPCONFIG_VERSION_PATCH 0
#define ESPCONFIG_VERSION "1.1.0"
#define ESPCONFIG_PROTOCOL_VERSION 4

namespace ESPConfig {

static constexpr const char* LibraryVersion = ESPCONFIG_VERSION;
static constexpr int ProtocolVersion = ESPCONFIG_PROTOCOL_VERSION;

} // namespace ESPConfig
