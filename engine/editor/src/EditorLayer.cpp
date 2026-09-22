#include "EditorLayer.hpp"
#include "Collider.hpp"
#include "EditorLogSink.hpp"
#include "Log.h"
#include "panels/ConsolePanel.hpp"
#include "panels/EntityInspectorPanel.hpp"
#include "panels/SceneHierarchyPanel.hpp"
#include "panels/SceneViewportPanel.hpp"
#include "panels/StatsPanel.hpp"
#include "panels/ToolbarPanel.hpp"
#include <algorithm>
#include <imgui-SFML.h>
#include <imgui.h>

namespace Elysium {

EditorLayer::EditorLayer() = default;

EditorLayer::~EditorLayer() {
  if (m_isInitialized) {
    Shutdown();
  }
}

void EditorLayer::Init(sf::RenderWindow &window) {
  if (m_isInitialized)
    return;

  m_window = &window;

  // Initialize ImGui-SFML binding
  if (!ImGui::SFML::Init(window)) {
    ELYSIUM_CORE_ERROR("Failed to initialize ImGui-SFML");
    return;
  }

  ImGuiIO &io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

  // Hook custom memory log sink into spdlog
  auto sink = GetEditorLogSink();
  if (Log::GetCoreLogger()) {
    Log::GetCoreLogger()->sinks().push_back(sink);
  }
  if (Log::GetClientLogger()) {
    Log::GetClientLogger()->sinks().push_back(sink);
  }

  // Apply professional editor styling
  ApplyModernDarkTheme();

  // Instantiate standard editor panels
  AddPanel<ToolbarPanel>();
  AddPanel<SceneViewportPanel>();
  AddPanel<SceneHierarchyPanel>();
  AddPanel<EntityInspectorPanel>();
  AddPanel<ConsolePanel>();
  AddPanel<StatsPanel>();

  m_isInitialized = true;
  ELYSIUM_CORE_INFO("Elysium EditorLayer initialized with Dear ImGui Docking");
}

void EditorLayer::ProcessEvent(const sf::Event &event) {
  if (!m_isInitialized || !m_window)
    return;

  ImGui::SFML::ProcessEvent(*m_window, event);

  // Global Editor Keyboard Shortcuts
  if (const auto *keyPressed = event.getIf<sf::Event::KeyPressed>()) {
    ImGuiIO &io = ImGui::GetIO();

    if (!io.WantTextInput) {
      // Space: Toggle Play / Pause
      if (keyPressed->code == sf::Keyboard::Key::Space) {
        if (m_context.playState == ScenePlayState::Play) {
          m_context.playState = ScenePlayState::Pause;
          ELYSIUM_INFO("Editor: Paused");
        } else {
          m_context.playState = ScenePlayState::Play;
          ELYSIUM_INFO("Editor: Playing");
        }
      }

      // 'F': Focus camera on selected entity
      if (keyPressed->code == sf::Keyboard::Key::F && m_context.selectedEntity) {
        m_context.camera.center.x = m_context.selectedEntity->position.x;
        m_context.camera.center.y = m_context.selectedEntity->position.y;
        ELYSIUM_INFO("Editor Camera focused on: {}", m_context.selectedEntity->name);
      }
    }
  }
}

void EditorLayer::BeginFrame(float dt) {
  if (!m_isInitialized || !m_window)
    return;

  ImGui::SFML::Update(*m_window, sf::seconds(dt));

  SetupDockSpace();
}

void EditorLayer::SetupDockSpace() {
  const ImGuiViewport *viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(viewport->WorkPos);
  ImGui::SetNextWindowSize(viewport->WorkSize);
  ImGui::SetNextWindowViewport(viewport->ID);

  ImGuiWindowFlags windowFlags = ImGuiWindowFlags_MenuBar |
                                ImGuiWindowFlags_NoDocking |
                                ImGuiWindowFlags_NoTitleBar |
                                ImGuiWindowFlags_NoCollapse |
                                ImGuiWindowFlags_NoResize |
                                ImGuiWindowFlags_NoMove |
                                ImGuiWindowFlags_NoBringToFrontOnFocus |
                                ImGuiWindowFlags_NoNavFocus;

  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

  ImGui::Begin("ElysiumDockSpaceHost", nullptr, windowFlags);
  ImGui::PopStyleVar(3);

  ImGuiID dockspaceId = ImGui::GetID("ElysiumEditorDockSpace");
  ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
}

void EditorLayer::RenderMenuBar(Scene &scene) {
  if (ImGui::BeginMenuBar()) {
    // 1. File Menu
    if (ImGui::BeginMenu("File")) {
      if (ImGui::MenuItem("New Scene", "Ctrl+N")) {
        scene.objects.clear();
        m_context.selectedEntity = nullptr;
        ELYSIUM_INFO("New Scene created");
      }
      if (ImGui::MenuItem("Open Scene...", "Ctrl+O")) {
        ELYSIUM_INFO("Open Scene dialog requested");
      }
      if (ImGui::MenuItem("Save Scene", "Ctrl+S")) {
        ELYSIUM_INFO("Scene saved");
      }
      if (ImGui::MenuItem("Save Scene As...", "Ctrl+Shift+S")) {
        ELYSIUM_INFO("Scene saved as...");
      }
      ImGui::Separator();
      if (ImGui::MenuItem("Exit", "Alt+F4")) {
        if (m_window) {
          m_window->close();
        }
      }
      ImGui::EndMenu();
    }

    // 2. Edit Menu
    if (ImGui::BeginMenu("Edit")) {
      if (ImGui::MenuItem("Undo", "Ctrl+Z", false, false)) {}
      if (ImGui::MenuItem("Redo", "Ctrl+Y", false, false)) {}
      ImGui::Separator();
      bool hasSelection = (m_context.selectedEntity != nullptr);
      if (ImGui::MenuItem("Delete Selected Entity", "Del", false, hasSelection)) {
        if (m_context.selectedEntity) {
          if (m_context.selectedEntity->rigidBody) {
            scene.physicsWorld.RemoveBody(m_context.selectedEntity->rigidBody.get());
          }
          auto it = std::find(scene.objects.begin(), scene.objects.end(),
                              m_context.selectedEntity);
          if (it != scene.objects.end()) {
            scene.objects.erase(it);
          }
          m_context.selectedEntity = nullptr;
        }
      }
      if (ImGui::MenuItem("Deselect", nullptr, false, hasSelection)) {
        m_context.selectedEntity = nullptr;
      }
      ImGui::EndMenu();
    }

    // 3. Scene Menu
    if (ImGui::BeginMenu("Scene")) {
      if (ImGui::MenuItem("Play Simulation", "Space",
                          m_context.playState == ScenePlayState::Play)) {
        m_context.playState = ScenePlayState::Play;
      }
      if (ImGui::MenuItem("Pause Simulation", nullptr,
                          m_context.playState == ScenePlayState::Pause)) {
        m_context.playState = ScenePlayState::Pause;
      }
      if (ImGui::MenuItem("Single Frame Step", "Ctrl+Right")) {
        m_context.playState = ScenePlayState::Step;
      }
      ImGui::Separator();
      if (ImGui::MenuItem("Reset Camera View")) {
        m_context.camera.Reset();
      }
      if (ImGui::MenuItem("Clear All Objects")) {
        scene.objects.clear();
        m_context.selectedEntity = nullptr;
      }
      ImGui::EndMenu();
    }

    // 4. Entities Menu
    if (ImGui::BeginMenu("Entities")) {
      if (ImGui::MenuItem("Create Empty Entity")) {
        auto entity = std::make_shared<GameObject>("Empty Entity");
        entity->position = Vec3(m_context.camera.center.x, m_context.camera.center.y, 0.0f);
        scene.AddGameObject(entity);
        m_context.selectedEntity = entity;
      }
      if (ImGui::MenuItem("Create Dynamic Ball")) {
        auto entity = std::make_shared<GameObject>("Dynamic Ball");
        entity->position = Vec3(m_context.camera.center.x, m_context.camera.center.y, 0.0f);
        auto &rb = entity->CreateRigidBody();
        rb.AddColliders(Collider::CreateSphere(0.5f, 1.0f));
        scene.AddGameObject(entity);
        m_context.selectedEntity = entity;
      }
      if (ImGui::MenuItem("Create Dynamic Box")) {
        auto entity = std::make_shared<GameObject>("Dynamic Box");
        entity->position = Vec3(m_context.camera.center.x, m_context.camera.center.y, 0.0f);
        auto &rb = entity->CreateRigidBody();
        rb.AddColliders(Collider::CreateBox(Vec3(0.5f, 0.5f, 0.0f), 1.0f));
        scene.AddGameObject(entity);
        m_context.selectedEntity = entity;
      }
      if (ImGui::MenuItem("Create Static Platform")) {
        auto entity = std::make_shared<GameObject>("Static Platform");
        entity->position = Vec3(m_context.camera.center.x, m_context.camera.center.y, 0.0f);
        auto &rb = entity->CreateRigidBody();
        rb.isStatic = true;
        rb.mass = 0.0f;
        rb.inverseMass = 0.0f;
        rb.AddColliders(Collider::CreateBox(Vec3(3.0f, 0.3f, 0.0f), 0.0f));
        scene.AddGameObject(entity);
        m_context.selectedEntity = entity;
      }
      ImGui::EndMenu();
    }

    // 5. View Menu
    if (ImGui::BeginMenu("View")) {
      for (auto &panel : m_panels) {
        ImGui::MenuItem(panel->name.c_str(), nullptr, &panel->isOpen);
      }
      ImGui::EndMenu();
    }

    // 6. Options Menu
    if (ImGui::BeginMenu("Options")) {
      if (ImGui::BeginMenu("Theme")) {
        if (ImGui::MenuItem("Elysium Dark")) {
          ApplyModernDarkTheme();
        }
        if (ImGui::MenuItem("ImGui Classic")) {
          ImGui::StyleColorsClassic();
        }
        if (ImGui::MenuItem("ImGui Light")) {
          ImGui::StyleColorsLight();
        }
        ImGui::EndMenu();
      }
      ImGui::Separator();
      ImGui::MenuItem("Show Debug Grid", nullptr, &m_context.drawDebugGrid);
      ImGui::MenuItem("Show Colliders", nullptr, &m_context.drawPhysicsColliders);
      ImGui::MenuItem("Show AABBs", nullptr, &m_context.drawAABBs);
      ImGui::EndMenu();
    }

    ImGui::EndMenuBar();
  }
}

void EditorLayer::RenderPanels(Scene &scene, SimpleRenderer &renderer) {
  if (!m_isInitialized)
    return;

  // Render main top-level menu bar inside dockspace host window
  RenderMenuBar(scene);

  // Close the DockSpace host window so docking nodes and child windows render
  ImGui::End();

  // Update entity count
  m_context.stats.entityCount = static_cast<uint32_t>(scene.objects.size());

  // Render each registered panel
  for (auto &panel : m_panels) {
    if (panel->isOpen) {
      panel->OnImGuiRender(m_context, scene, renderer);
    }
  }
}

void EditorLayer::EndFrame() {
  if (!m_isInitialized || !m_window)
    return;

  // Render ImGui draw lists to the SFML render window
  ImGui::SFML::Render(*m_window);
}

void EditorLayer::Shutdown() {
  if (!m_isInitialized)
    return;

  m_panels.clear();
  ImGui::SFML::Shutdown();
  m_isInitialized = false;
  m_window = nullptr;
  ELYSIUM_CORE_INFO("Elysium EditorLayer shutdown complete");
}

void EditorLayer::ApplyModernDarkTheme() {
  ImGuiStyle &style = ImGui::GetStyle();
  style.WindowRounding = 5.0f;
  style.ChildRounding = 4.0f;
  style.FrameRounding = 3.0f;
  style.PopupRounding = 4.0f;
  style.ScrollbarRounding = 4.0f;
  style.GrabRounding = 3.0f;
  style.TabRounding = 4.0f;

  style.WindowBorderSize = 1.0f;
  style.FrameBorderSize = 0.5f;
  style.PopupBorderSize = 1.0f;

  ImVec4 *colors = style.Colors;
  colors[ImGuiCol_Text] = ImVec4(0.92f, 0.92f, 0.94f, 1.00f);
  colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.52f, 0.58f, 1.00f);
  colors[ImGuiCol_WindowBg] = ImVec4(0.12f, 0.12f, 0.14f, 1.00f);
  colors[ImGuiCol_ChildBg] = ImVec4(0.10f, 0.10f, 0.12f, 0.50f);
  colors[ImGuiCol_PopupBg] = ImVec4(0.14f, 0.14f, 0.16f, 0.98f);
  colors[ImGuiCol_Border] = ImVec4(0.25f, 0.26f, 0.30f, 0.60f);
  colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
  colors[ImGuiCol_FrameBg] = ImVec4(0.18f, 0.18f, 0.22f, 1.00f);
  colors[ImGuiCol_FrameBgHovered] = ImVec4(0.24f, 0.25f, 0.30f, 1.00f);
  colors[ImGuiCol_FrameBgActive] = ImVec4(0.28f, 0.30f, 0.36f, 1.00f);
  colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
  colors[ImGuiCol_TitleBgActive] = ImVec4(0.15f, 0.16f, 0.20f, 1.00f);
  colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.10f, 0.10f, 0.12f, 0.60f);
  colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.17f, 1.00f);
  colors[ImGuiCol_ScrollbarBg] = ImVec4(0.10f, 0.10f, 0.12f, 0.60f);
  colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.26f, 0.28f, 0.34f, 1.00f);
  colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.34f, 0.36f, 0.44f, 1.00f);
  colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.40f, 0.44f, 0.54f, 1.00f);
  colors[ImGuiCol_CheckMark] = ImVec4(0.35f, 0.65f, 1.00f, 1.00f);
  colors[ImGuiCol_SliderGrab] = ImVec4(0.35f, 0.65f, 1.00f, 1.00f);
  colors[ImGuiCol_SliderGrabActive] = ImVec4(0.45f, 0.75f, 1.00f, 1.00f);
  colors[ImGuiCol_Button] = ImVec4(0.20f, 0.22f, 0.28f, 1.00f);
  colors[ImGuiCol_ButtonHovered] = ImVec4(0.28f, 0.32f, 0.40f, 1.00f);
  colors[ImGuiCol_ButtonActive] = ImVec4(0.35f, 0.42f, 0.55f, 1.00f);
  colors[ImGuiCol_Header] = ImVec4(0.20f, 0.22f, 0.28f, 1.00f);
  colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.30f, 0.38f, 1.00f);
  colors[ImGuiCol_HeaderActive] = ImVec4(0.32f, 0.38f, 0.48f, 1.00f);
  colors[ImGuiCol_Separator] = ImVec4(0.25f, 0.26f, 0.30f, 0.60f);
  colors[ImGuiCol_SeparatorHovered] = ImVec4(0.35f, 0.40f, 0.50f, 1.00f);
  colors[ImGuiCol_SeparatorActive] = ImVec4(0.45f, 0.55f, 0.70f, 1.00f);
  colors[ImGuiCol_Tab] = ImVec4(0.14f, 0.14f, 0.18f, 1.00f);
  colors[ImGuiCol_TabHovered] = ImVec4(0.24f, 0.28f, 0.36f, 1.00f);
  colors[ImGuiCol_TabActive] = ImVec4(0.20f, 0.24f, 0.32f, 1.00f);
  colors[ImGuiCol_TabUnfocused] = ImVec4(0.12f, 0.12f, 0.15f, 1.00f);
  colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.16f, 0.18f, 0.24f, 1.00f);
  colors[ImGuiCol_DockingPreview] = ImVec4(0.35f, 0.65f, 1.00f, 0.70f);
  colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
}

} // namespace Elysium
