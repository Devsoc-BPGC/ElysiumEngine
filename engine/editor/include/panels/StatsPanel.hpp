#ifndef STATSPANEL_HPP
#define STATSPANEL_HPP

#include "EditorPanel.hpp"
#include <array>

namespace Elysium {

class StatsPanel : public EditorPanel {
public:
  StatsPanel();
  ~StatsPanel() override = default;

  void OnImGuiRender(EditorContext &context, Scene &scene,
                     SimpleRenderer &renderer) override;

private:
  std::array<float, 60> m_fpsHistory{};
  size_t m_historyIndex = 0;
};

} // namespace Elysium

#endif // STATSPANEL_HPP
