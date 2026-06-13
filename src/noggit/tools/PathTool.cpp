// This file is part of Noggit3, licensed under GNU General Public License (version 3).

#include "PathTool.hpp"

#include <noggit/ActionManager.hpp>
#include <noggit/Input.hpp>
#include <noggit/MapView.h>
#include <noggit/ui/CurrentTexture.h>
#include <noggit/ui/PathToolWidget.hpp>
#include <noggit/ui/texture_palette_small.hpp>
#include <noggit/ui/TexturePicker.h>
#include <noggit/ui/TexturingGUI.h>
#include <noggit/ui/tools/UiCommon/ImageMaskSelector.hpp>
#include <noggit/ui/tools/ToolPanel/ToolPanel.hpp>
#include <noggit/ui/windows/noggitWindow/NoggitWindow.hpp>
#include <noggit/World.h>
#include <noggit/Brush.h>

#include <QDockWidget>
#include <QMenu>
#include <QSettings>

namespace Noggit
{
  PathTool::PathTool(MapView* mapView)
    : Tool{ mapView }
    , _show_texture_palette_window(false)
    , _show_texture_browser_window(false)
  {
    addHotkey("toggleTexturePalette"_hash, Hotkey{
        .onPress = [=] { _show_texture_palette_window.toggle(); },
        .condition = [=] { return mapView->get_editing_mode() == editing_mode::path && !NOGGIT_CUR_ACTION; },
        });
  }

  PathTool::~PathTool()
  {
    delete _texturePaletteDock;
    delete _textureBrowserDock;
    delete _pathToolWidget;
  }

  char const* PathTool::name() const
  {
    return "Path Tool";
  }

  editing_mode PathTool::editingMode() const
  {
    return editing_mode::path;
  }

  Ui::FontNoggit::Icons PathTool::icon() const
  {
    return Ui::FontNoggit::TOOL_PATH;
  }

  void PathTool::setupUi(Ui::Tools::ToolPanel* toolPanel)
  {
    auto mv = mapView();
    _pathToolWidget = new Ui::PathToolWidget(mv, &_show_texture_palette_window, mv);
    toolPanel->registerTool(this, _pathToolWidget);

    QObject::connect(_pathToolWidget, &Ui::PathToolWidget::texturePaletteToggled,
        [=]()
        {
            _show_texture_palette_window.set(!_show_texture_palette_window.get());
        });

    QObject::connect(_pathToolWidget, &Ui::PathToolWidget::textureBrowserToggled,
        [=]()
        {
            _show_texture_browser_window.set(!_show_texture_browser_window.get());
        });

    setupTextureBrowser(mv);
    setupTexturePalette(mv);
  }

  void PathTool::setupTextureBrowser(MapView* mv)
  {
    _textureBrowserDock = new QDockWidget("Texture Browser", mv);
    _textureBrowserDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetClosable);
    _textureBrowserDock->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea | Qt::LeftDockWidgetArea);
    mv->mainWindow()->addDockWidget(Qt::BottomDockWidgetArea, _textureBrowserDock);
    _textureBrowserDock->hide();

    QObject::connect(_textureBrowserDock, &QDockWidget::visibilityChanged, [=](bool visible) {
      if (mv->isUiHidden()) return;
      mv->settings()->setValue("map_view/texture_browser", visible);
      mv->settings()->sync();
    });

    QObject::connect(mv, &QObject::destroyed, _textureBrowserDock, &QObject::deleteLater);

    _textureBrowser = new Noggit::Ui::tileset_chooser(mv);
    _textureBrowserDock->setWidget(_textureBrowser);
    QObject::connect(mv, &QObject::destroyed, _textureBrowser, &QObject::deleteLater);

    QObject::connect(_textureBrowser, &Noggit::Ui::tileset_chooser::selected, [=](std::string const& filename) {
      mv->makeCurrent();
      OpenGL::context::scoped_setter const _(::gl, mv->context());
      Noggit::Ui::selected_texture::set({ filename, mv->getRenderContext() });
    });

    QObject::connect(&_show_texture_browser_window, &Noggit::BoolToggleProperty::changed, [this, mv] {
      if (mv->get_editing_mode() != editing_mode::path || mv->isUiHidden()) {
        QSignalBlocker const _(_show_texture_browser_window);
        _show_texture_browser_window.set(false);
        return;
      }
      QSignalBlocker const _(_textureBrowser);
      _textureBrowserDock->setVisible(_show_texture_browser_window.get());
    });
  }

  void PathTool::setupTexturePalette(MapView* mv)
  {
    _texturePalette = new Noggit::Ui::texture_palette_small(mv->project(), mv->getWorld()->getMapID(), mv);
    _texturePaletteDock = new QDockWidget("Texture Palette", mv);
    _texturePaletteDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetClosable);
    _texturePaletteDock->setWidget(_texturePalette);
    _texturePaletteDock->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea);
    _texturePaletteDock->hide();
    QObject::connect(mv, &QObject::destroyed, _texturePaletteDock, &QObject::deleteLater);
    mv->mainWindow()->addDockWidget(Qt::BottomDockWidgetArea, _texturePaletteDock);

    QObject::connect(_texturePaletteDock, &QDockWidget::visibilityChanged, [=](bool visible) {
      if (mv->isUiHidden()) return;
      mv->settings()->setValue("map_view/texture_palette", visible);
      mv->settings()->sync();
    });

    QObject::connect(_texturePalette, &Noggit::Ui::texture_palette_small::selected, [=](std::string const& filename) {
      mv->makeCurrent();
      OpenGL::context::scoped_setter const _(::gl, mv->context());
      Noggit::Ui::selected_texture::set({ filename, mv->getRenderContext() });
    });

    QObject::connect(mv, &QObject::destroyed, _texturePalette, &QObject::deleteLater);

    QObject::connect(&_show_texture_palette_window, &Noggit::BoolToggleProperty::changed, _texturePaletteDock, [=] {
      if (mv->get_editing_mode() != editing_mode::path || mv->isUiHidden()) {
        QSignalBlocker const _(_show_texture_palette_window);
        _show_texture_palette_window.set(false);
        return;
      }
      QSignalBlocker const _(_texturePalette);
      _texturePaletteDock->setVisible(_show_texture_palette_window.get());
    });
  }

  void PathTool::registerMenuItems(QMenu* menu)
  {
    addMenuTitle(menu, "Path Tool");
    addMenuItem(menu, "Texture Browser", Qt::Key_X, _show_texture_browser_window);
    addMenuItem(menu, "Texture palette", _show_texture_palette_window);
  }

  ToolDrawParameters PathTool::drawParameters() const
  {
    return ToolDrawParameters
    {
      .radius = _pathToolWidget->getRadius(),
      .inner_radius = _pathToolWidget->getHardness(),
      .cursor_type = CursorType::CIRCLE,
      .terrain_type = eTerrainType_Smooth,
    };
  }

  void PathTool::onSelected()
  {
    auto mv = mapView();
    auto showTextureBrowser = _show_texture_browser_window.get() || mv->settings()->value("map_view/texture_browser", false).toBool();
    auto showTexturePalette = _show_texture_palette_window.get() || mv->settings()->value("map_view/texture_palette", false).toBool();
    _textureBrowserDock->setVisible(!mv->isUiHidden() && showTextureBrowser);
    _texturePaletteDock->setVisible(!mv->isUiHidden() && showTexturePalette);
  }

  void PathTool::onDeselected()
  {
    QSignalBlocker const blocker1(_show_texture_palette_window);
    QSignalBlocker const blocker2(_show_texture_browser_window);
    QSignalBlocker const blocker3(_textureBrowserDock);
    _textureBrowserDock->setVisible(false);
    _texturePaletteDock->setVisible(false);
  }

  void PathTool::onTick(float deltaTime, TickParameters const& params)
  {
    if (!params.left_mouse) return;

    auto mv = mapView();

    if (!!Noggit::Ui::selected_texture::get())
    {
      if ((params.displayMode == display_mode::in_3D && !params.underMap) || params.displayMode == display_mode::in_2D)
      {
        auto action = NOGGIT_ACTION_MGR->beginAction(mv, Noggit::ActionFlags::eCHUNKS_TERRAIN | Noggit::ActionFlags::eCHUNKS_TEXTURE,
            Noggit::ActionModalityControllers::eLMB);

        float radius = _pathToolWidget->getRadius();
        float hardness = _pathToolWidget->getHardness();
        float depthSpeed = _pathToolWidget->getDepthSpeed();
        float brushLevel = _pathToolWidget->getBrushLevel();

        // Change terrain (e.g. sinking)
        mv->getWorld()->changeTerrain(mv->cursorPosition(), deltaTime * depthSpeed, radius, eTerrainType_Smooth, hardness);
        
        // Paint texture
        Brush texture_brush;
        texture_brush.init();
        texture_brush.setRadius(radius);
        texture_brush.setHardness(hardness);

        float alpha_target = brushLevel;
        float strength = 1.0f - pow(1.0f - 0.5f, deltaTime * 10.0f);
        
        if (!_pathToolWidget->getImageMaskSelector()->isEnabled())
        {
          mv->getWorld()->paintTexture(mv->cursorPosition(), &texture_brush, alpha_target, strength, *Noggit::Ui::selected_texture::get());
        }
        else
        {
          mv->getWorld()->stampTexture(mv->cursorPosition(), &texture_brush, alpha_target, strength, *Noggit::Ui::selected_texture::get(), _pathToolWidget->getMaskImage(), _pathToolWidget->getImageMaskSelector()->getBrushMode());
        }
      }
    }
  }

  void PathTool::onMouseMove(MouseMoveParameters const& params)
  {
    // Optionally add shortcut combinations (alt+scroll, etc) if needed later
  }

  void PathTool::hidePopups()
  {
    _texturePalette->hide();
    _textureBrowserDock->hide();
  }
}
