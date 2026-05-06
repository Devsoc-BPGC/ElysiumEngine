#include "Application.hpp"
#include <iostream>

namespace Elysium {

    Application::Application() {
    }

    Application::~Application() {
    }

    void Application::Run() {
        std::cout << "Elysium Application is running..." << std::endl;
        // The actual loop will be added later when we integrate windowing.
    }

}
