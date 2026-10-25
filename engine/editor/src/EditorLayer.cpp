#include "EditorLayer.hpp"
#include "Application.hpp"
#include "AssetManager.hpp"
#include "Collider.hpp"
#include "EditorLogSink.hpp"
#include "FileDialog.hpp"
#include "Log.h"
#include "Project.hpp"
#include "panels/ConsolePanel.hpp"
#include "panels/ContentBrowserPanel.hpp"
#include "panels/EntityInspectorPanel.hpp"
#include "panels/SceneHierarchyPanel.hpp"
#include "panels/SceneViewportPanel.hpp"
#include "panels/StatsPanel.hpp"
#include "panels/ToolbarPanel.hpp"
#include <algorithm>
#include <imgui-SFML.h>
#include <imgui.h>
#include "AudioSourceComponent.hpp"

namespace Elysium {

EditorLayer::EditorLayer() : Layer("EditorLayer") {
  // Configure default scene
  m_scene.physicsWorld.gravity = Vec3(0.0f, 9.81f, 0.0f);
}

EditorLayer::~EditorLayer() {
  if (m_isInitialized) {
    Shutdown();
  }
}

void EditorLayer::OnAttach() {
  if (!m_isInitialized) {
    m_window = &Application::Get().GetWindow().GetNativeWindow();
    Init(*m_window);
  }
}

void EditorLayer::OnDetach() { Shutdown(); }

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

  // Initialize Project / Asset Registry
  if (auto project = Project::GetActive()) {
    AssetManager::Get().Init(project->GetAssetDirectory());
  } else {
    // Default fallback project initialization or assets directory
    AssetManager::Get().Init("assets");
  }

  // Instantiate standard editor panels
  AddPanel<ToolbarPanel>();
  AddPanel<SceneViewportPanel>();
  AddPanel<SceneHierarchyPanel>();
  AddPanel<EntityInspectorPanel>();
  AddPanel<ContentBrowserPanel>();
  AddPanel<ConsolePanel>();
  AddPanel<StatsPanel>();

  m_isInitialized = true;
  ELYSIUM_CORE_INFO("Elysium EditorLayer attached with Dear ImGui Docking");
}

void EditorLayer::OnUpdate(Timestep ts) {
  m_lastDt = ts.GetSeconds();

  sf::Clock updateTimer;
  if (m_context.playState == ScenePlayState::Play) {
    m_scene.physicsWorld.boundaryRotation += 0.0005f;
    m_scene.Update(ts.GetSeconds());
  } else if (m_context.playState == ScenePlayState::Step) {
    m_scene.Update(1.0f / 60.0f);
    m_context.playState = ScenePlayState::Pause;
  }

  float updateMs = updateTimer.getElapsedTime().asSeconds() * 1000.0f;
  m_context.stats.updateTimeMs = updateMs;
  m_context.stats.physicsTimeMs = updateMs * 0.75f;

  m_context.stats.entityCount = static_cast<uint32_t>(m_scene.objects.size());
  m_context.stats.frameTimeMs = ts.GetMilliseconds();
  m_context.stats.fps =
      (ts.GetSeconds() > 0.0001f) ? (1.0f / ts.GetSeconds()) : 60.0f;
  m_context.stats.drawCalls = m_renderer.drawCalls;
}

void EditorLayer::OnImGuiRender() {
  if (!m_isInitialized || !m_window)
    return;

  sf::Clock renderTimer;

  ImGui::SFML::Update(*m_window, sf::seconds(m_lastDt));

  SetupDockSpace();
  RenderMenuBar(m_scene);
  ImGui::End();

  for (auto &panel : m_panels) {
    if (panel->isOpen) {
      panel->OnImGuiRender(m_context, m_scene, m_renderer);
    }
  }

  RenderProjectModals();

  ImGui::SFML::Render(*m_window);

  m_context.stats.renderTimeMs =
      renderTimer.getElapsedTime().asSeconds() * 1000.0f;
}

void EditorLayer::OnEvent(Event &event) {
  EventDispatcher dispatcher(event);
  dispatcher.Dispatch<KeyPressedEvent>(
      [this](KeyPressedEvent &e) { return OnKeyPressed(e); });
}

bool EditorLayer::OnKeyPressed(KeyPressedEvent &e) {
  ImGuiIO &io = ImGui::GetIO();
  if (io.WantTextInput)
    return false;

  // Space: Toggle Play / Pause
  if (e.GetKeyCode() == sf::Keyboard::Key::Space) {
    if (m_context.playState == ScenePlayState::Play) {
      m_context.playState = ScenePlayState::Pause;
      ELYSIUM_INFO("Editor: Paused");
    } else {
      m_context.playState = ScenePlayState::Play;
      ELYSIUM_INFO("Editor: Playing");
    }
    return true;
  }

  // 'F': Focus camera on selected entity
  if (e.GetKeyCode() == sf::Keyboard::Key::F && m_context.selectedEntity) {
    m_context.camera.center.x = m_context.selectedEntity->position.x;
    m_context.camera.center.y = m_context.selectedEntity->position.y;
    ELYSIUM_INFO("Editor Camera focused on: {}",
                 m_context.selectedEntity->name);
    return true;
  }

  // Delete: Delete selected entity
  if (e.GetKeyCode() == sf::Keyboard::Key::Delete && m_context.selectedEntity) {
    if (m_context.selectedEntity->rigidBody) {
      m_scene.physicsWorld.RemoveBody(
          m_context.selectedEntity->rigidBody.get());
    }
    auto it = std::find(m_scene.objects.begin(), m_scene.objects.end(),
                        m_context.selectedEntity);
    if (it != m_scene.objects.end()) {
      m_scene.objects.erase(it);
    }
    m_context.selectedEntity = nullptr;
    return true;
  }

  return false;
}

void EditorLayer::OnProjectLoaded() {
  auto project = Project::GetActive();
  if (!project) return;

  const auto &config = project->GetConfig();
  std::filesystem::path assetsDir = project->GetAssetDirectory();

  // 1. Reload Asset Manager with project asset directory
  AssetManager::Get().ReloadProjectAssets(assetsDir);

  // 2. Update window title
  if (m_window) {
    m_window->setTitle("Elysium Game Engine — " + config.Name + " [" + config.ProjectDirectory.string() + "]");
  }

  ELYSIUM_INFO("Editor: Switched to Project '{}' ({})", config.Name, config.ProjectDirectory.string());
}

void EditorLayer::SetupDockSpace() {
  const ImGuiViewport *viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(viewport->WorkPos);
  ImGui::SetNextWindowSize(viewport->WorkSize);
  ImGui::SetNextWindowViewport(viewport->ID);

  ImGuiWindowFlags windowFlags =
      ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking |
      ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
      ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
      ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

  std::string hostWindowName = "ElysiumDockSpaceHost";
  if (auto project = Project::GetActive()) {
    hostWindowName += " - " + project->GetConfig().Name;
  }

  ImGui::Begin(hostWindowName.c_str(), nullptr, windowFlags);
  ImGui::PopStyleVar(3);

  ImGuiID dockspaceId = ImGui::GetID("ElysiumEditorDockSpace");
  ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
}

void EditorLayer::NewProjectDialog() {
  std::filesystem::path defaultDir = std::filesystem::current_path();
  if (auto proj = Project::GetActive()) {
    defaultDir = proj->GetProjectDirectory().parent_path();
  }

  auto selectedFolder = FileDialog::SelectFolder("Select Project Root Directory", defaultDir);
  if (selectedFolder.has_value()) {
    std::filesystem::path folderPath = *selectedFolder;
    std::string projName = folderPath.filename().string();
    if (projName.empty()) {
      projName = "NewProject";
    }

    auto project = Project::NewProject(folderPath, projName);
    if (project) {
      OnProjectLoaded();
    } else {
      ELYSIUM_ERROR("Failed to create new project at '{}'", folderPath.string());
    }
  }
}

void EditorLayer::OpenProjectDialog() {
  std::filesystem::path defaultDir = std::filesystem::current_path();
  if (auto proj = Project::GetActive()) {
    defaultDir = proj->GetProjectDirectory();
  }

  std::vector<FileDialogFilter> filters = {
      {"Elysium Project (*.elyproj)", "*.elyproj"}};

  auto selectedFile = FileDialog::OpenFile("Open Elysium Project", defaultDir, filters);
  if (selectedFile.has_value()) {
    std::filesystem::path projFile = *selectedFile;
    auto project = Project::LoadProject(projFile);
    if (project) {
      OnProjectLoaded();
    } else {
      ELYSIUM_ERROR("Failed to open project file at '{}'", projFile.string());
    }
  }
}

void EditorLayer::RenderMenuBar(Scene &scene) {
  if (ImGui::BeginMenuBar()) {
    // 1. File Menu
    if (ImGui::BeginMenu("File")) {
      if (ImGui::MenuItem("New Project...", "Ctrl+Shift+N")) {
        NewProjectDialog();
      }
      if (ImGui::MenuItem("Open Project...", "Ctrl+Shift+O")) {
        OpenProjectDialog();
      }
      if (ImGui::MenuItem("Save Project", nullptr, false, Project::GetActive() != nullptr)) {
        Project::SaveActiveProject();
        AssetManager::Get().SaveManifest();
        ELYSIUM_INFO("Project and Manifest saved");
      }
      ImGui::Separator();

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
        Application::Get().Close();
      }
      ImGui::EndMenu();
    }

    // 2. Edit Menu
    if (ImGui::BeginMenu("Edit")) {
      if (ImGui::MenuItem("Undo", "Ctrl+Z", false, false)) {
      }
      if (ImGui::MenuItem("Redo", "Ctrl+Y", false, false)) {
      }
      ImGui::Separator();
      bool hasSelection = (m_context.selectedEntity != nullptr);
      if (ImGui::MenuItem("Delete Selected Entity", "Del", false,
                          hasSelection)) {
        if (m_context.selectedEntity) {
          if (m_context.selectedEntity->rigidBody) {
            scene.physicsWorld.RemoveBody(
                m_context.selectedEntity->rigidBody.get());
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
        entity->position =
            Vec3(m_context.camera.center.x, m_context.camera.center.y, 0.0f);
        scene.AddGameObject(entity);
        m_context.selectedEntity = entity;
      }
      if (ImGui::MenuItem("Create Rigid Body")) {
        auto entity = std::make_shared<GameObject>("Rigid Body");
        entity->position =
            Vec3(m_context.camera.center.x, m_context.camera.center.y, 0.0f);
        auto &rb = entity->CreateRigidBody();
        scene.AddGameObject(entity);
        m_context.selectedEntity = entity;
      }
      if (ImGui::MenuItem("Create Audio Source")) {
        auto entity = std::make_shared<GameObject>("Audio Source");
        entity->position =
            Vec3(m_context.camera.center.x, m_context.camera.center.y, 0.0f);
        auto &as = entity->AddComponent<AudioSourceComponent>();
        scene.AddGameObject(entity);
        m_context.selectedEntity = entity;
        ELYSIUM_INFO("Added AudioSourceComponent to entity: {}", entity->GetComponent<AudioSourceComponent>() != nullptr);
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
      ImGui::MenuItem("Show Colliders", nullptr,
                      &m_context.drawPhysicsColliders);
      ImGui::MenuItem("Show AABBs", nullptr, &m_context.drawAABBs);
      ImGui::EndMenu();
    }

    ImGui::EndMenuBar();
  }
}

void EditorLayer::RenderProjectModals() {
  // 1. New Project Modal Dialog
  if (m_ShowNewProjectModal) {
    ImGui::OpenPopup("New Project");
  }

  ImVec2 center = ImGui::GetMainViewport()->GetCenter();
  ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
  ImGui::SetNextWindowSize(ImVec2(520.0f, 220.0f), ImGuiCond_Appearing);

  if (ImGui::BeginPopupModal("New Project", &m_ShowNewProjectModal, ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::Text("Create a new Elysium Engine project.");
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::InputText("Project Name", m_NewProjectNameBuffer, sizeof(m_NewProjectNameBuffer));

    // Update default path suggestion if user modifies name
    if (ImGui::IsItemDeactivatedAfterEdit()) {
      std::string suggestedPath = "projects/" + std::string(m_NewProjectNameBuffer);
      std::strncpy(m_NewProjectPathBuffer, suggestedPath.c_str(), sizeof(m_NewProjectPathBuffer) - 1);
    }

    ImGui::InputText("Directory", m_NewProjectPathBuffer, sizeof(m_NewProjectPathBuffer));
    ImGui::SameLine();
    if (ImGui::Button("Browse...##NewDir")) {
      auto sel = FileDialog::SelectFolder("Select Directory for Project", m_NewProjectPathBuffer);
      if (sel.has_value()) {
        std::strncpy(m_NewProjectPathBuffer, sel->string().c_str(), sizeof(m_NewProjectPathBuffer) - 1);
      }
    }
    ImGui::TextDisabled("Structure: <Directory>/Assets, <Directory>/Assets/Scenes, assets.meta");

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (ImGui::Button("Create", ImVec2(120.0f, 0.0f))) {
      std::filesystem::path dirPath = m_NewProjectPathBuffer;
      std::string name = m_NewProjectNameBuffer;

      auto project = Project::NewProject(dirPath, name);
      if (project) {
        OnProjectLoaded();
      }
      m_ShowNewProjectModal = false;
      ImGui::CloseCurrentPopup();
    }

    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(120.0f, 0.0f))) {
      m_ShowNewProjectModal = false;
      ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
  }

  // 2. Open Project Modal Dialog
  if (m_ShowOpenProjectModal) {
    ImGui::OpenPopup("Open Project");
  }

  ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
  ImGui::SetNextWindowSize(ImVec2(560.0f, 240.0f), ImGuiCond_Appearing);

  if (ImGui::BeginPopupModal("Open Project", &m_ShowOpenProjectModal, ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::Text("Enter or browse for an existing .elyproj file:");
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::InputText("Project File (.elyproj)", m_OpenProjectPathBuffer, sizeof(m_OpenProjectPathBuffer));
    ImGui::SameLine();
    if (ImGui::Button("Browse...##OpenFile")) {
      auto sel = FileDialog::OpenFile("Open Project File", "", {{"Elysium Project (*.elyproj)", "*.elyproj"}});
      if (sel.has_value()) {
        std::strncpy(m_OpenProjectPathBuffer, sel->string().c_str(), sizeof(m_OpenProjectPathBuffer) - 1);
      }
    }

    // Display available projects found in working directory or subdirectories
    ImGui::Spacing();
    ImGui::TextDisabled("Quick Select Found Projects:");
    std::error_code ec;
    int foundCount = 0;
    for (const auto &entry : std::filesystem::recursive_directory_iterator(".", ec)) {
      if (entry.is_regular_file() && entry.path().extension() == ".elyproj") {
        std::string pStr = entry.path().lexically_normal().string();
        if (ImGui::SmallButton(pStr.c_str())) {
          std::strncpy(m_OpenProjectPathBuffer, pStr.c_str(), sizeof(m_OpenProjectPathBuffer) - 1);
        }
        foundCount++;
        if (foundCount >= 5) break;
      }
    }
    if (foundCount == 0) {
      ImGui::TextDisabled("  (No .elyproj files discovered in working directory)");
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (ImGui::Button("Open", ImVec2(120.0f, 0.0f))) {
      std::filesystem::path projPath = m_OpenProjectPathBuffer;
      auto project = Project::LoadProject(projPath);
      if (project) {
        OnProjectLoaded();
      }
      m_ShowOpenProjectModal = false;
      ImGui::CloseCurrentPopup();
    }

    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(120.0f, 0.0f))) {
      m_ShowOpenProjectModal = false;
      ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
  }
}

void EditorLayer::Shutdown() {
  if (!m_isInitialized)
    return;

  AssetManager::Get().Shutdown();
  m_panels.clear();
  ImGui::SFML::Shutdown();
  m_isInitialized = false;
  m_window = nullptr;
  ELYSIUM_CORE_INFO("Elysium EditorLayer detached");
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
