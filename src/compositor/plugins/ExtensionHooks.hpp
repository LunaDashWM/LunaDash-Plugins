#pragma once
#include "compositor/layout/WindowLayout.hpp"
#include <string>
#include <vector>
namespace LunaDash {
class PluginManager;
struct WindowTemplate;
Json windowAnimationProfile(PluginManager &plugins, const Json &preferences,
                            const WindowTemplate &windowTemplate);
int shellAnimationDuration(PluginManager &plugins, int fallback);
std::string windowTemplateKey(const PluginManager &plugins);
Json initialWindowRule(PluginManager &plugins, const Json &context,
                       int workspace, bool maximized, int workspaceCount);
std::vector<WindowPlacement>
pluginWindowPlacements(PluginManager &plugins,
                       const WindowTemplate &windowTemplate,
                       LayoutWorkspaceId workspace, Rect area,
                       const std::vector<WindowPlacement> &placements);
} // namespace LunaDash
