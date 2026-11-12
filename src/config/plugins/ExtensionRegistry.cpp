#include "config/plugins/ExtensionRegistry.hpp"
#include "core/settings/SettingsSchema.hpp"
#include <algorithm>
#include <cmath>

namespace LunaDash {
Json extensionTargets() {
  static const auto targets = [] {
    for (const char *path : {LUDASH_PLUGIN_TARGETS_SOURCE_FILE,
                             LUDASH_PLUGIN_TARGETS_INSTALL_FILE}) {
      auto document = readJsonFile(path);
      if (!document.is_discarded())
        return jsonAsArray(document);
    }
    return Json::array();
  }();
  return targets;
}
Json extensionTarget(const std::string &id) {
  for (const auto &value : extensionTargets())
    if (jsonString(value, "id") == id)
      return jsonAsObject(value);
  return Json::object();
}
Json extensionDefaults(const Json &schema) {
  return Settings::defaults(schema);
}
bool validateExtensionSettings(const Json &schema, const Json &settings,
                               std::string *error) {
  return Settings::validateSchema(schema, error) &&
         Settings::validateValues(schema, settings, error);
}
bool resolveExtensionOptions(const Json &target, const Json &implementation,
                             const std::string &mode, Json *options,
                             std::string *error) {
  const auto fail = [error](const std::string &message) {
    if (error)
      *error = message;
    return false;
  };
  const auto schema = jsonObject(target, "options");
  const auto removed = jsonObject(target, "removedOptions");
  const auto replaceOnly = jsonObject(target, "replaceOnly");
  Json declared = Json::object();
  for (auto it = removed.begin(); it != removed.end(); ++it)
    if (jsonContains(implementation, it.key()))
      return fail(it.key() + " was removed; " + jsonAsString(it.value()));
  for (const auto &other : extensionTargets())
    for (const auto *section : {"options", "removedOptions"}) {
      const auto names = jsonObject(other, section);
      for (auto it = names.begin(); it != names.end(); ++it)
        if (jsonContains(implementation, it.key()) &&
            !jsonContains(schema, it.key()))
          return fail(it.key() + " is not an option of target " +
                      jsonString(target, "id"));
    }
  for (auto it = schema.begin(); it != schema.end(); ++it)
    if (jsonContains(implementation, it.key()))
      declared[it.key()] = jsonValue(implementation, it.key());
  if (!Settings::validateSchema(schema, error) ||
      !Settings::validateValues(schema, declared, error))
    return false;
  auto resolved = extensionDefaults(schema);
  for (auto it = declared.begin(); it != declared.end(); ++it)
    resolved[it.key()] = it.value();
  for (auto it = replaceOnly.begin(); it != replaceOnly.end(); ++it)
    if (mode != "replace" &&
        Settings::Detail::arrayContains(jsonAsArray(it.value()),
                                        jsonValue(resolved, it.key())))
      return fail(it.key() + " " + jsonAsString(jsonValue(resolved, it.key())) +
                  " requires replacement mode");
  if (options)
    *options = resolved;
  return true;
}
bool validateExtensionResponse(const Json &schema, const Json &value) {
  const auto type = jsonString(schema, "type");
  if (jsonContains(schema, "enum") &&
      !Settings::Detail::arrayContains(jsonArray(schema, "enum"), value))
    return false;
  const auto inRange = [&](double number) {
    return std::isfinite(number) &&
           (!jsonContains(schema, "minimum") ||
            number >= jsonAsDouble(jsonValue(schema, "minimum"))) &&
           (!jsonContains(schema, "maximum") ||
            number <= jsonAsDouble(jsonValue(schema, "maximum")));
  };
  if (type == "object") {
    const auto properties = jsonObject(schema, "properties");
    if (!value.is_object() || value.size() != properties.size())
      return false;
    for (auto it = properties.begin(); it != properties.end(); ++it)
      if (!jsonContains(value, it.key()) ||
          !validateExtensionResponse(it.value(), jsonValue(value, it.key())))
        return false;
    return true;
  }
  if (type == "array") {
    const auto items = jsonObject(schema, "items");
    return value.is_array() &&
           std::all_of(value.begin(), value.end(), [&](const Json &item) {
             return validateExtensionResponse(items, item);
           });
  }
  if (type == "integer")
    return value.is_number() && inRange(jsonAsDouble(value)) &&
           jsonAsDouble(value) == std::floor(jsonAsDouble(value));
  if (type == "number")
    return value.is_number() && inRange(jsonAsDouble(value));
  if (type == "string")
    return value.is_string();
  if (type == "boolean")
    return value.is_boolean();
  return false;
}
} // namespace LunaDash
