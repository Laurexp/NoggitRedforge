// This file is part of Noggit3, licensed under GNU General Public License (version 3).

#define _USE_MATH_DEFINES
#include <cmath>

#include "PathToolWidget.hpp"

#include <noggit/BoolToggleProperty.hpp>
#include <noggit/MapView.h>
#include <noggit/ui/CurrentTexture.h>
#include <noggit/ui/tools/UiCommon/ExtendedSlider.hpp>
#include <noggit/ui/tools/UiCommon/ImageMaskSelector.hpp>
#include <noggit/ui/tools/UiCommon/expanderwidget.h>
#include <noggit/ui/TexturingGUI.h>
#include <noggit/World.h>

#include <QtWidgets/QFormLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>

namespace Noggit
{
  namespace Ui
  {
    PathToolWidget::PathToolWidget(MapView* map_view, BoolToggleProperty* show_quick_palette, QWidget* parent)
      : QWidget(parent)
      , _map_view(map_view)
    {
      setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
      auto layout = new QVBoxLayout(this);
      layout->setAlignment(Qt::AlignTop);

      auto current_texture_widget = new current_texture(true, this);
      current_texture_widget->resize(QSize(225, 225));
      layout->addWidget(current_texture_widget);
      layout->setAlignment(current_texture_widget, Qt::AlignHCenter);

      auto tool_widget = new QWidget(this);
      auto form_layout = new QFormLayout(tool_widget);
      layout->addWidget(tool_widget);

      _radius_slider = new Tools::UiCommon::ExtendedSlider(this);
      _radius_slider->setPrefix("");
      _radius_slider->setRange(0.0, 1000.0);
      _radius_slider->setDecimals(2);
      _radius_slider->setValue(15.0);
      form_layout->addRow("Radius:", _radius_slider);

      _hardness_slider = new Tools::UiCommon::ExtendedSlider(this);
      _hardness_slider->setPrefix("");
      _hardness_slider->setRange(0.0, 1.0);
      _hardness_slider->setDecimals(2);
      _hardness_slider->setSingleStep(0.05f);
      _hardness_slider->setValue(0.5f);
      form_layout->addRow("Hardness:", _hardness_slider);

      _depth_speed_slider = new Tools::UiCommon::ExtendedSlider(this);
      _depth_speed_slider->setPrefix("");
      _depth_speed_slider->setRange(-10.0, 10.0);
      _depth_speed_slider->setDecimals(2);
      _depth_speed_slider->setValue(-2.0);
      form_layout->addRow("Depth/Speed:", _depth_speed_slider);

      _brush_level_slider = new Tools::UiCommon::ExtendedSlider(this);
      _brush_level_slider->setPrefix("");
      _brush_level_slider->setRange(0.0, 255.0);
      _brush_level_slider->setDecimals(0);
      _brush_level_slider->setValue(255.0);
      form_layout->addRow("Opacity:", _brush_level_slider);

      _image_mask_group = new Noggit::Ui::Tools::ImageMaskSelector(map_view, this);
      _image_mask_group->setContinuousActionName("Paint");
      _image_mask_group->setBrushModeVisible(false);
      _mask_image = _image_mask_group->getPixmap()->toImage();
      _image_mask_group->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);

      auto* customBrushBox = new ExpanderWidget(this);
      customBrushBox->setExpanderTitle("Custom Brush");
      customBrushBox->addPage(_image_mask_group);
      customBrushBox->setExpanded(false);
      layout->addWidget(customBrushBox);
      layout->setAlignment(customBrushBox, Qt::AlignTop);

      connect(_image_mask_group, &Noggit::Ui::Tools::ImageMaskSelector::rotationUpdated, this, &PathToolWidget::updateMaskImage);
      connect(_radius_slider, &Tools::UiCommon::ExtendedSlider::valueChanged, this, &PathToolWidget::updateMaskImage);
      connect(_image_mask_group, &Noggit::Ui::Tools::ImageMaskSelector::pixmapUpdated, this, &PathToolWidget::updateMaskImage);

      auto quick_palette_btn = new QPushButton("Quick Palette", this);
      layout->addWidget(quick_palette_btn);
      layout->setAlignment(quick_palette_btn, Qt::AlignTop);

      connect(quick_palette_btn, &QPushButton::clicked, [this]() {
        emit texturePaletteToggled();
      });

      // Synchronize with Noggit::Ui::selected_texture
      connect(current_texture_widget, &current_texture::texture_dropped, [this, map_view](std::string const& filename) {
        map_view->makeCurrent();
        OpenGL::context::scoped_setter const _(::gl, map_view->context());
        Noggit::Ui::selected_texture::set({ filename, map_view->getRenderContext() });
      });

      connect(current_texture_widget, &current_texture::clicked, [this]() {
        emit textureBrowserToggled();
      });

      // Ensure the widget shows the currently selected texture when updated elsewhere
      connect(current_texture_widget, &current_texture::texture_updated, [map_view]() {
        map_view->getWorld()->notifyTileRendererOnSelectedTextureChange();
      });

      setMinimumWidth(250);
      setMaximumWidth(250);
    }

    PathToolWidget::~PathToolWidget()
    {
    }

    float PathToolWidget::getRadius() const
    {
      return static_cast<float>(_radius_slider->value());
    }

    float PathToolWidget::getHardness() const
    {
      return static_cast<float>(_hardness_slider->value());
    }

    float PathToolWidget::getDepthSpeed() const
    {
      return static_cast<float>(_depth_speed_slider->value());
    }

    float PathToolWidget::getBrushLevel() const
    {
      return static_cast<float>(_brush_level_slider->value());
    }

    void PathToolWidget::updateMaskImage()
    {
      QPixmap* pixmap = _image_mask_group->getPixmap();
      QTransform matrix;
      matrix.rotateRadians(_image_mask_group->getRotation() * M_PI / 180.f);
      _mask_image = pixmap->toImage().transformed(matrix, Qt::SmoothTransformation);

      emit _map_view->trySetBrushTexture(&_mask_image, this);
    }
  }
}
