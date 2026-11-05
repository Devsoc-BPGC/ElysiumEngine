#pragma once

#include "Script.hpp"
#include <functional>
#include <memory>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace Elysium {

/**
 * @class ScriptRegistry
 * @brief Central registry mapping script names to factory instantiation functions.
 */
class ScriptRegistry {
public:
    using ScriptFactory = std::function<std::shared_ptr<Script>()>;

    /**
     * @brief Registers a C++ Script class with a unique name identifier.
     * @tparam T Script type derived from Elysium::Script.
     * @param name Unique name to identify this script in the editor and scene files.
     */
    template <typename T>
    static void Register(const std::string& name) {
        static_assert(std::is_base_of<Script, T>::value, "T must derive from Elysium::Script");
        GetFactoryMap()[name] = []() -> std::shared_ptr<Script> {
            return std::make_shared<T>();
        };
    }

    /**
     * @brief Instantiates a script by registered name.
     * @param name Registered script name.
     * @return Shared pointer to new Script instance, or nullptr if unregistered.
     */
    static std::shared_ptr<Script> Instantiate(const std::string& name);

    /**
     * @brief Checks if a script name is registered in the engine.
     * @param name Script name.
     * @return true if registered, false otherwise.
     */
    static bool Exists(const std::string& name);

    /**
     * @brief Retrieves all registered script names (used for Editor dropdowns).
     * @return Vector of script names.
     */
    static std::vector<std::string> GetAvailableScripts();

private:
    static std::unordered_map<std::string, ScriptFactory>& GetFactoryMap();
};

} // namespace Elysium
