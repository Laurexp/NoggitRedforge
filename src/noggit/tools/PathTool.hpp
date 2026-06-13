// This file is part of Noggit3, licensed under GNU General Public License (version 3).

#pragma once

#include <noggit/Tool.hpp>
#include <noggit/BoolToggleProperty.hpp>

class QDockWidget;

namespace Noggit
{
  namespace Ui
  {
    class PathToolWidget;
    class texture_palette_small;
    class texture_picker;
    struct tileset_chooser;
  }

  class PathTool final : public Tool
  {
  public:
    PathTool(MapView* mapView);
    ~PathTool() override;

    char const* name() const override;
    editing_mode editingMode() const override;
    Ui::FontNoggit::Icons icon() const override;
    void setupUi(Ui::Tools::ToolPanel* toolPanel) override;
    ToolDrawParameters drawParameters() const override;

    void onSelected() override;
    void onDeselected() override;
    void onTick(float deltaTime, TickParameters const& params) override;
    void onMouseMove(MouseMoveParameters const& params) override;

    void registerMenuItems(QMenu* menu) override;

  private:
    void setupTextureBrowser(MapView* mv);
    void setupTexturePalette(MapView* mv);

    void hidePopups();

    Ui::PathToolWidget* _pathToolWidget = nullptr;

    BoolToggleProperty _show_texture_palette_window;
    BoolToggleProperty _show_texture_browser_window;

    QDockWidget* _textureBrowserDock = nullptr;
    QDockWidget* _texturePaletteDock = nullptr;

    Ui::tileset_chooser* _textureBrowser = nullptr;
    Ui::texture_palette_small* _texturePalette = nullptr;
  };
}
