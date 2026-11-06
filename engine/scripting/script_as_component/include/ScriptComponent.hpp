#pragma once

#include "Component.hpp"
#include "Script.hpp"
#include <memory>
#include <string>

namespace Elysium {

class Entity;

/**
 * @class ScriptComponent
 * @brief Component that attaches an executable script instance to a GameObject.
 */
class ScriptComponent : public Component {
public:
    /**
     * @brief Default constructor (unbound script).
     */
    ScriptComponent() = default;

    /**
     * @brief Constructs a ScriptComponent bound to a registered script name.
     * @param scriptName Name of the registered script class.
     */
    explicit ScriptComponent(const std::string& scriptName);

    /**
     * @brief Virtual destructor ensuring clean script shutdown.
     */
    ~ScriptComponent() override;

    /**
     * @brief Returns a copy of this component for Play mode scene cloning.
     */
    std::unique_ptr<Component> Clone() const override {
        auto clone = std::make_unique<ScriptComponent>(m_ScriptName);
        return clone;
    }

    /**
     * @brief Re-binds the script when the scene is cloned for Play mode.
     */
    void OnCloned(const std::shared_ptr<GameObject> &owner) override {
        (void)owner;
        if (!m_ScriptName.empty()) {
            BindScript(m_ScriptName);
        }
    }

    /**
     * @brief Called by the engine when the scene starts or component is created.
     */
    void Start() override;

    /**
     * @brief Called each simulation frame to advance script logic.
     * @param dt Delta time in seconds.
     */
    void Update(float dt) override;

    /**
     * @brief Binds and instantiates a script by registered name.
     * @param name Registered script identifier.
     * @return true if successfully bound, false otherwise.
     */
    bool BindScript(const std::string& name);

    /**
     * @brief Unbinds and destroys the current script instance safely.
     */
    void UnbindScript();

    /**
     * @brief Reloads the current script instance (hot-reloading).
     */
    void ReloadScript();

    /**
     * @brief Checks if a script is currently instantiated and active.
     * @return true if bound and active.
     */
    bool IsBound() const { return m_Instance != nullptr; }

    /**
     * @brief Gets the name of the assigned script.
     * @return Script name string.
     */
    const std::string& GetScriptName() const { return m_ScriptName; }

    /**
     * @brief Gets the raw script instance pointer.
     * @return Pointer to Script, or nullptr if unbound.
     */
    Script* GetInstance() const { return m_Instance.get(); }

private:
    std::string m_ScriptName;
    std::shared_ptr<Script> m_Instance = nullptr;
    std::shared_ptr<Entity> m_EntityHandle = nullptr;
    bool m_Started = false;
};

} // namespace Elysium
