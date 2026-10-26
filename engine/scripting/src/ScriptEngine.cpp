#include "ScriptEngine.hpp"
#include "ScriptingWorld.hpp"
#include "Log.h"

namespace Elysium {

static bool s_Initialized = false;

/**
 * @brief Initializes the scripting subsystem.
 */
void ScriptEngine::Init() {
    if (s_Initialized) {
        ELYSIUM_CORE_WARN("ScriptEngine already initialized!");
        return;
    }
    s_Initialized = true;
    ELYSIUM_CORE_INFO("ScriptEngine subsystem initialized.");
}

/**
 * @brief Shuts down the scripting subsystem.
 */
void ScriptEngine::Shutdown() {
    if (!s_Initialized) return;

    SetActiveScene(nullptr);
    s_Initialized = false;
    ELYSIUM_CORE_INFO("ScriptEngine subsystem shut down.");
}

/**
 * @brief Called when a scene starts.
 * @param scene Active scene pointer.
 */
void ScriptEngine::OnSceneStart(Scene* scene) {
    SetActiveScene(scene);
}

/**
 * @brief Called when a scene stops.
 */
void ScriptEngine::OnSceneStop() {
    SetActiveScene(nullptr);
}

/**
 * @brief Updates scripts for the current frame.
 * @param dt Delta time in seconds.
 */
void ScriptEngine::OnUpdate(float dt) {
    (void)dt;
}

/**
 * @brief Checks if the scripting subsystem is initialized.
 * @return true if initialized, false otherwise.
 */
bool ScriptEngine::IsInitialized() {
    return s_Initialized;
}

} // namespace Elysium
