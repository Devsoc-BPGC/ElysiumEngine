#include "ScriptRegistry.hpp"
#include <algorithm>

namespace Elysium {

std::unordered_map<std::string, ScriptRegistry::ScriptFactory>& ScriptRegistry::GetFactoryMap() {
    static std::unordered_map<std::string, ScriptFactory> s_Factories;
    return s_Factories;
}

std::shared_ptr<Script> ScriptRegistry::Instantiate(const std::string& name) {
    auto& factories = GetFactoryMap();
    auto it = factories.find(name);
    if (it != factories.end()) {
        return it->second();
    }
    return nullptr;
}

bool ScriptRegistry::Exists(const std::string& name) {
    auto& factories = GetFactoryMap();
    return factories.find(name) != factories.end();
}

std::vector<std::string> ScriptRegistry::GetAvailableScripts() {
    std::vector<std::string> scripts;
    auto& factories = GetFactoryMap();
    scripts.reserve(factories.size());

    for (const auto& [name, _] : factories) {
        scripts.push_back(name);
    }
    std::sort(scripts.begin(), scripts.end());
    return scripts;
}

} // namespace Elysium
