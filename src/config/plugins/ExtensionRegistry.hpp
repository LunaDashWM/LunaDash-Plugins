#pragma once
#include "core/json/JsonAccess.hpp"

#include <string>

namespace LunaDash {
Json extensionTargets();
Json extensionTarget(const std::string &id);
bool validateExtensionSettings(const Json &schema, const Json &settings,
                               std::string *error);
Json extensionDefaults(const Json &schema);
// Resolves the manifest options a target declares in the registry; options of
// other targets, removed options and replace-only values are rejected.
bool resolveExtensionOptions(const Json &target, const Json &implementation,
                             const std::string &mode, Json *options,
                             std::string *error);
// Checks a hook result against a target's registry `response` schema: objects
// must have exactly the listed properties; arrays, integers, numbers, strings
// and booleans support items, minimum/maximum and enum. A missing or unknown
// schema rejects every result.
bool validateExtensionResponse(const Json &schema, const Json &value);
} // namespace LunaDash
