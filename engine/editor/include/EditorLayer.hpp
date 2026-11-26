#ifndef EDITORLAYER_HPP
#define EDITORLAYER_HPP

#include "EditorContext.hpp"
#include "EditorPanel.hpp"
#include "KeyEvent.hpp"
#include "Layer.hpp"
#include "ProjectHubPanel.hpp"
#include "RecentProjects.hpp"
#include "Scene.hpp"
#include "SimpleRenderer.hpp"
#include "Timestep.hpp"
#include <SFML/Graphics/RenderWindow.hpp>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace Elysium {

class ContentBrowserPanel;
class GamePreviewPanel;
class SceneViewportPanel;
class ToolbarPanel;
class Window;

enum class EditorMode {
  ProjectHub,
  Workspace
};

class EditorLayer : public Layer {
public:
  EditorLayer();
  ~EditorLayer() override;

  void OnAttach() override;
  void OnDetach() override;
  void OnUpdate(Timestep ts) override;
  void OnImGuiRender() override;
  void OnEvent(Event &event) override;

  void Init(sf::RenderWindow &window);
  void Shutdown();

  EditorMode GetMode() const { return m_mode; }
  RecentProjectsManager &GetRecentProjects() { return m_recentProjects; }
  const RecentProjectsManager &GetRecentProjects() const { return m_recentProjects; }
  ProjectHubPanel *GetProjectHubPanel() { return m_projectHubPanel.get(); }

  bool OpenProject(const std::filesystem::path &projectFilePath,
                   std::string *outError = nullptr);
  bool CreateNewProject(const NewProjectRequest &request,
                        std::string *outError = nullptr);
  void SwitchToProjectHub();

  EditorContext &GetContext() { return m_context; }
  const EditorContext &GetContext() const { return m_context; }

  Scene &GetScene() { return m_scene; }
  SimpleRenderer &GetRenderer() { return m_renderer; }

  template <typename T, typename... Args> T &AddPanel(Args &&...args) {
    auto panel = std::make_unique<T>(std::forward<Args>(args)...);
    T &ref = *panel;
    m_panels.push_back(std::move(panel));
    return ref;
  }

  bool IsInitialized() const { return m_isInitialized; }

  void SetupDockSpace();
  void RenderMenuBar(Scene &scene);
  void RenderProjectModals();
  void ApplyModernDarkTheme();

  void OnProjectLoaded();
  void ResetLayout();
  void NewProjectDialog();
  void OpenProjectDialog();

  bool OnKeyPressed(KeyPressedEvent &e);

  void ResetWorkspaceScene();
  bool InitImGui(sf::RenderWindow &window);
  void ApplyPendingWindowReconfigure();

  enum class PendingWindowMode { None, ProjectHub, Workspace };

  EditorMode m_mode = EditorMode::ProjectHub;
  PendingWindowMode m_PendingWindowMode = PendingWindowMode::None;
  bool m_logSinkAttached = false;
  Window *m_appWindow = nullptr;
  sf::RenderWindow *m_window = nullptr;
  EditorContext m_context;
  RecentProjectsManager m_recentProjects;
  std::unique_ptr<ProjectHubPanel> m_projectHubPanel;
  std::vector<std::unique_ptr<EditorPanel>> m_panels;
  bool m_isInitialized = false;

  // Project Modals State
  bool m_ShowNewProjectModal = false;
  bool m_ShowOpenProjectModal = false;
  char m_NewProjectNameBuffer[128] = "MyProject";
  char m_NewProjectPathBuffer[512] = "projects/MyProject";
  char m_OpenProjectPathBuffer[512] = "";

  Scene m_scene;
  std::unique_ptr<Scene> m_runtimeScene; // Simulated copy of m_scene in Play
  ToolbarPanel *m_toolbar = nullptr;
  SceneViewportPanel *m_sceneViewport = nullptr;
  GamePreviewPanel *m_gamePreview = nullptr;
  ContentBrowserPanel *m_contentBrowser = nullptr;
  bool m_gamePreviewWasOpen = false; // The Toolbar opens along with it
  Scene *m_activeSceneBeforePlay = nullptr; // Restored when Play stops
  SimpleRenderer m_renderer{50.0f};
  float m_lastDt = 0.016f;

  std::string m_ImGuiIniPath;
  bool m_ResetLayoutScheduled = false;
};

} // namespace Elysium

#endif // EDITORLAYER_HPP
