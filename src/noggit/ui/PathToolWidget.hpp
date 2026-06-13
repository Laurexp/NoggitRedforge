// This file is part of Noggit3, licensed under GNU General Public License (version 3).

#pragma once

#include <QWidget>
#include <QImage>

class MapView;

namespace Noggit
{
  struct BoolToggleProperty;

  namespace Ui
  {
    namespace Tools
    {
      class ImageMaskSelector;
    }
    namespace Tools::UiCommon
    {
      class ExtendedSlider;
    }

    class PathToolWidget : public QWidget
    {
      Q_OBJECT

    public:
      PathToolWidget(MapView* map_view, BoolToggleProperty* show_quick_palette, QWidget* parent = nullptr);
      ~PathToolWidget();

      float getRadius() const;
      float getHardness() const;
      float getDepthSpeed() const;
      float getBrushLevel() const;

      Tools::ImageMaskSelector* getImageMaskSelector() const { return _image_mask_group; }
      QImage* getMaskImage() { return &_mask_image; }

    public slots:
      void updateMaskImage();

    signals:
      void texturePaletteToggled();
      void textureBrowserToggled();

    private:
      MapView* _map_view;
      Tools::ImageMaskSelector* _image_mask_group;
      QImage _mask_image;
      Tools::UiCommon::ExtendedSlider* _radius_slider;
      Tools::UiCommon::ExtendedSlider* _hardness_slider;
      Tools::UiCommon::ExtendedSlider* _depth_speed_slider;
      Tools::UiCommon::ExtendedSlider* _brush_level_slider;
      float _brush_level = 255.0f;
    };
  }
}
