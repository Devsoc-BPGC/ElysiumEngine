#include "ScriptComponent.hpp"
#include "ScriptRegistry.hpp"
#include "ScriptingWorld.hpp"
#include "Scene.hpp"
#include "Entity.hpp"
#include "GameObject.hpp"
#include "Log.h"

namespace Elysium {

ScriptComponent::ScriptComponent(const std::string& scriptName) {
    BindScript(scriptName);
}

ScriptComponent::~ScriptComponent() {
    UnbindScript();
}

bool ScriptComponent::BindScript(const std::string& name) {
    if (m_Instance) {
        UnbindScript();
    }

    m_ScriptName = name;
    if (name.empty()) {
        return false;
    }

    m_Instance = ScriptRegistry::Instantiate(name);
    if (!m_Instance) {
        ELYSIUM_CORE_WARN("ScriptComponent: Failed to bind script '{}' - not registered.", name);
        return false;
    }

    if (gameObject) {
        std::shared_ptr<GameObject> sharedObj = nullptr;
        if (auto* scene = GetActiveScene()) {
            auto it = scene->m_EntityMap.find(gameObject->GetUUID());
            if (it != scene->m_EntityMap.end()) {
                sharedObj = it->second;
            }
        }

        if (!sharedObj) {
            sharedObj = std::shared_ptr<GameObject>(gameObject, [](GameObject*){});
        }

        m_EntityHandle = std::make_shared<Entity>(sharedObj);
        m_Instance->entity = m_EntityHandle.get();
        m_Instance->m_EntityHolder = m_EntityHandle;
        m_Instance->gameObject = gameObject;

        m_Instance->OnCreate();
        m_Started = true;
    }

    ELYSIUM_CORE_INFO("ScriptComponent: Successfully bound script '{}'.", name);
    return true;
}

void ScriptComponent::UnbindScript() {
    if (m_Instance) {
        m_Instance->OnDestroy();
        m_Instance->entity = nullptr;
        m_Instance->m_EntityHolder.reset();
        m_Instance.reset();
        m_EntityHandle.reset();
        m_Started = false;
    }
}

void ScriptComponent::ReloadScript() {
    std::string current = m_ScriptName;
    UnbindScript();
    BindScript(current);
}

void ScriptComponent::Start() {
    if (m_Instance && !m_Started && gameObject) {
        std::shared_ptr<GameObject> sharedObj = nullptr;
        if (auto* scene = GetActiveScene()) {
            auto it = scene->m_EntityMap.find(gameObject->GetUUID());
            if (it != scene->m_EntityMap.end()) {
                sharedObj = it->second;
            }
        }

        if (!sharedObj) {
            sharedObj = std::shared_ptr<GameObject>(gameObject, [](GameObject*){});
        }

        m_EntityHandle = std::make_shared<Entity>(sharedObj);
        m_Instance->entity = m_EntityHandle.get();
        m_Instance->m_EntityHolder = m_EntityHandle;
        m_Instance->gameObject = gameObject;

        m_Instance->OnCreate();
        m_Started = true;
    }
}

void ScriptComponent::Update(float dt) {
    if (m_Instance && m_Started) {
        m_Instance->OnUpdate(dt);
    }
}

} // namespace Elysium
