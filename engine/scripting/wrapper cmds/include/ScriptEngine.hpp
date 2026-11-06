#pragma once

#include "Core.hpp"

class Scene;

namespace Elysium {

/**
 * @class ScriptEngine
 * @brief Subsystem managing script execution and lifecycle.
 */
class ELYSIUM_API ScriptEngine {
public:
    /**
     * @brief Initializes the scripting subsystem.
     */
    static void Init();

    /**
     * @brief Shuts down the scripting subsystem.
     */
    static void Shutdown();

    /**
     * @brief Called when a scene starts.
     * @param scene Active scene pointer.
     */
    static void OnSceneStart(Scene* scene);

    /**
     * @brief Called when a scene stops.
     */
    static void OnSceneStop();

    /**
     * @brief Updates scripts for the current frame.
     * @param dt Delta time in seconds.
     */
    static void OnUpdate(float dt);

    /**
     * @brief Checks if the scripting subsystem is initialized.
     * @return true if initialized, false otherwise.
     */
    static bool IsInitialized();
};

} // namespace Elysium
