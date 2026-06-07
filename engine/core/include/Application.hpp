/**
 * @file Application.hpp
 * @brief Base application class and client factory function definition.
 */

#pragma once

#include "Core.hpp"

namespace Elysium {

    /**
     * @class Application
     * @brief The core application class that manages the main loop and engine subsystems.
     *
     * Client applications should inherit from this class and implement the CreateApplication()
     * factory function.
     */
    class ELYSIUM_API Application {
    public:
        /**
         * @brief Constructs the Application object.
         */
        Application();

        /**
         * @brief Virtual destructor for proper cleanup of derived classes.
         */
        virtual ~Application();

        /**
         * @brief The main execution loop of the application.
         *
         * This method is called by the entry point and runs until the application is closed.
         */
        virtual void Run();
    };

    /**
     * @brief Factory function to be implemented by the client application.
     * @return A pointer to a new instance of a class derived from Application.
     *
     * The engine's entry point calls this function to bootstrap the application.
     */
    Application* CreateApplication();

}
