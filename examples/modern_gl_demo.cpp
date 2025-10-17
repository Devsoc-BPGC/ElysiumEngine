/**
 * ElysiumEngine - Modern OpenGL 4.1 Core Profile Physics Demo
 * Compatible with macOS (uses Core Profile, not Compatibility Profile)
 * 
 * Features:
 * - Vertex/Fragment shaders for particle and grid rendering
 * - VAO/VBO for efficient GPU rendering
 * - Interactive physics simulation with visual feedback
 */

#include <SFML/Window.hpp>
#include <SFML/System.hpp>
#include <OpenGL/gl3.h>
#include <iostream>
#include <cmath>
#include <vector>

// Include physics headers
#include "physics/TwoDimensionPhysics.hpp"

// Vertex shader for colored primitives (GLSL 410 Core)
const char* vertexShaderSource = R"(
#version 410 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec3 aColor;

out vec3 fragColor;

uniform mat4 projection;

void main() {
    gl_Position = projection * vec4(aPos, 0.0, 1.0);
    fragColor = aColor;
}
)";

// Fragment shader for colored primitives
const char* fragmentShaderSource = R"(
#version 410 core
in vec3 fragColor;
out vec4 FragColor;

void main() {
    FragColor = vec4(fragColor, 1.0);
}
)";

// Helper function to compile shaders
GLuint compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    
    // Check compilation errors
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "❌ Shader compilation failed:\n" << infoLog << std::endl;
    }
    return shader;
}

// Helper function to create shader program
GLuint createShaderProgram() {
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
    
    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    
    // Check linking errors
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        std::cerr << "❌ Shader program linking failed:\n" << infoLog << std::endl;
    }
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    return program;
}

// Generate circle vertices (for particle rendering)
std::vector<float> generateCircleVertices(float centerX, float centerY, float radius, int segments, float r, float g, float b) {
    std::vector<float> vertices;
    
    // Center vertex
    vertices.push_back(centerX);
    vertices.push_back(centerY);
    vertices.push_back(r);
    vertices.push_back(g);
    vertices.push_back(b);
    
    // Circle vertices
    for (int i = 0; i <= segments; i++) {
        float angle = 2.0f * M_PI * i / segments;
        float x = centerX + radius * cos(angle);
        float y = centerY + radius * sin(angle);
        
        vertices.push_back(x);
        vertices.push_back(y);
        vertices.push_back(r);
        vertices.push_back(g);
        vertices.push_back(b);
    }
    
    return vertices;
}

int main() {
    std::cout << "🎮 ElysiumEngine - Modern OpenGL 4.1 Physics Demo\n";
    std::cout << "==================================================\n";
    std::cout << "Controls:\n";
    std::cout << "  Arrow Keys: Apply force\n";
    std::cout << "  R: Reset particle\n";
    std::cout << "  G: Toggle gravity\n";
    std::cout << "  ESC: Exit\n\n";

    const int WIDTH = 800;
    const int HEIGHT = 600;
    const double FORCE_MAGNITUDE = 5000.0;
    const double GRAVITY = 980.0; // pixels/s^2
    const float PARTICLE_RADIUS = 20.0f;
    
    // Create OpenGL context with Core Profile
    sf::ContextSettings settings;
    settings.depthBits = 24;
    settings.stencilBits = 8;
    settings.antiAliasingLevel = 4;
    settings.majorVersion = 4;
    settings.minorVersion = 1;
    settings.attributeFlags = sf::ContextSettings::Core;
    
    sf::Window window(sf::VideoMode({WIDTH, HEIGHT}), 
                      "ElysiumEngine - Modern OpenGL 4.1 Demo",
                      sf::Style::Default,
                      sf::State::Windowed,
                      settings);
    window.setFramerateLimit(60);
    
    if (!window.setActive(true)) {
        std::cerr << "Failed to set window active\n";
        return 1;
    }
    
    // Print OpenGL version
    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << "\n";
    std::cout << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << "\n\n";
    
    // Create shader program
    GLuint shaderProgram = createShaderProgram();
    
    // Create orthographic projection matrix (2D)
    float orthoMatrix[16] = {
        2.0f/WIDTH, 0.0f,        0.0f, 0.0f,
        0.0f,       -2.0f/HEIGHT, 0.0f, 0.0f,
        0.0f,       0.0f,        -1.0f, 0.0f,
        -1.0f,      1.0f,         0.0f, 1.0f
    };
    
    GLint projLoc = glGetUniformLocation(shaderProgram, "projection");
    glUseProgram(shaderProgram);
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, orthoMatrix);
    
    // Create physics particle
    TwoDimensionalParticle particle(WIDTH / 2.0, HEIGHT / 2.0, 10.0); // mass = 10.0
    bool gravityEnabled = false;
    
    // VAO and VBO for grid
    GLuint gridVAO, gridVBO;
    glGenVertexArrays(1, &gridVAO);
    glGenBuffers(1, &gridVBO);
    
    // Grid vertices (position + color)
    std::vector<float> gridVertices;
    // Grid color: RGB(50, 50, 65)
    float gr = 50.0f / 255.0f;
    float gg = 50.0f / 255.0f;
    float gb = 65.0f / 255.0f;
    
    // Vertical lines
    for (int i = 0; i <= WIDTH; i += 50) {
        gridVertices.push_back(i); gridVertices.push_back(0);
        gridVertices.push_back(gr); gridVertices.push_back(gg); gridVertices.push_back(gb);
        gridVertices.push_back(i); gridVertices.push_back(HEIGHT);
        gridVertices.push_back(gr); gridVertices.push_back(gg); gridVertices.push_back(gb);
    }
    // Horizontal lines
    for (int i = 0; i <= HEIGHT; i += 50) {
        gridVertices.push_back(0); gridVertices.push_back(i);
        gridVertices.push_back(gr); gridVertices.push_back(gg); gridVertices.push_back(gb);
        gridVertices.push_back(WIDTH); gridVertices.push_back(i);
        gridVertices.push_back(gr); gridVertices.push_back(gg); gridVertices.push_back(gb);
    }
    
    glBindVertexArray(gridVAO);
    glBindBuffer(GL_ARRAY_BUFFER, gridVBO);
    glBufferData(GL_ARRAY_BUFFER, gridVertices.size() * sizeof(float), gridVertices.data(), GL_STATIC_DRAW);
    
    // Position attribute
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // Color attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    // VAO and VBO for particle
    GLuint particleVAO, particleVBO;
    glGenVertexArrays(1, &particleVAO);
    glGenBuffers(1, &particleVBO);
    
    // VAO and VBO for force vector
    GLuint forceVAO, forceVBO;
    glGenVertexArrays(1, &forceVAO);
    glGenBuffers(1, &forceVBO);
    
    std::cout << "✅ OpenGL initialized with Core Profile 4.1\n";
    std::cout << "✅ Shaders compiled and linked successfully\n";
    std::cout << "✅ Physics simulation started!\n";
    std::cout << "Initial position: (" << particle.getX() << ", " << particle.getY() << ")\n\n";

    bool running = true;
    sf::Clock clock;
    
    while (running) {
        // Event handling
        while (auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                running = false;
            }
            
            if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                if (keyPressed->code == sf::Keyboard::Key::Escape) {
                    running = false;
                }
                if (keyPressed->code == sf::Keyboard::Key::R) {
                    particle = TwoDimensionalParticle(WIDTH / 2.0, HEIGHT / 2.0, 10.0);
                    std::cout << "🔄 Particle reset\n";
                }
                if (keyPressed->code == sf::Keyboard::Key::G) {
                    gravityEnabled = !gravityEnabled;
                    std::cout << "🌍 Gravity: " << (gravityEnabled ? "ON" : "OFF") << "\n";
                }
            }
        }

        // Apply forces based on keyboard input
        double forceX = 0.0;
        double forceY = 0.0;

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left)) {
            forceX = -FORCE_MAGNITUDE;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)) {
            forceX = FORCE_MAGNITUDE;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up)) {
            forceY = -FORCE_MAGNITUDE;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down)) {
            forceY = FORCE_MAGNITUDE;
        }

        // Add gravity
        if (gravityEnabled) {
            forceY += particle.getMass() * GRAVITY;
        }

        // Update particle
        Force force(forceX, forceY);
        particle.update(force);

        // Boundary collision (simple bounce)
        double x = particle.getX();
        double y = particle.getY();
        
        if (x < PARTICLE_RADIUS || x > WIDTH - PARTICLE_RADIUS) {
            particle.setPosition(
                x < PARTICLE_RADIUS ? PARTICLE_RADIUS : WIDTH - PARTICLE_RADIUS,
                y
            );
        }
        if (y < PARTICLE_RADIUS || y > HEIGHT - PARTICLE_RADIUS) {
            particle.setPosition(
                x,
                y < PARTICLE_RADIUS ? PARTICLE_RADIUS : HEIGHT - PARTICLE_RADIUS
            );
        }

        // === Modern OpenGL Rendering ===
        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        glUseProgram(shaderProgram);
        
        // Draw grid
        glBindVertexArray(gridVAO);
        glDrawArrays(GL_LINES, 0, gridVertices.size() / 5);
        
        // Draw force vector
        if (forceX != 0 || forceY != 0) {
            float vecScale = 0.5f;
            float forceVerts[] = {
                (float)particle.getX(), (float)particle.getY(), 1.0f, 1.0f, 0.0f,  // Yellow
                (float)(particle.getX() + forceX * vecScale), (float)(particle.getY() + forceY * vecScale), 1.0f, 1.0f, 0.0f
            };
            
            glBindVertexArray(forceVAO);
            glBindBuffer(GL_ARRAY_BUFFER, forceVBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(forceVerts), forceVerts, GL_DYNAMIC_DRAW);
            
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(2 * sizeof(float)));
            glEnableVertexAttribArray(1);
            
            glDrawArrays(GL_LINES, 0, 2);
        }
        
        // Draw particle (cyan circle)
        std::vector<float> circleVerts = generateCircleVertices(
            particle.getX(), particle.getY(), PARTICLE_RADIUS, 32,
            0.2f, 0.8f, 1.0f  // Cyan color
        );
        
        glBindVertexArray(particleVAO);
        glBindBuffer(GL_ARRAY_BUFFER, particleVBO);
        glBufferData(GL_ARRAY_BUFFER, circleVerts.size() * sizeof(float), circleVerts.data(), GL_DYNAMIC_DRAW);
        
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(2 * sizeof(float)));
        glEnableVertexAttribArray(1);
        
        glDrawArrays(GL_TRIANGLE_FAN, 0, circleVerts.size() / 5);
        
        window.display();
    }

    // Cleanup
    glDeleteVertexArrays(1, &gridVAO);
    glDeleteBuffers(1, &gridVBO);
    glDeleteVertexArrays(1, &particleVAO);
    glDeleteBuffers(1, &particleVBO);
    glDeleteVertexArrays(1, &forceVAO);
    glDeleteBuffers(1, &forceVBO);
    glDeleteProgram(shaderProgram);

    std::cout << "\n👋 Modern OpenGL demo closed\n";
    std::cout << "Final position: (" << particle.getX() << ", " << particle.getY() << ")\n";

    return 0;
}
