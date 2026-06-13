// This file is part of Noggit3, licensed under GNU General Public License (version 3).

#include "ImageGenerateLightmapNode.hpp"

#include <noggit/ui/tools/NodeEditor/Nodes/BaseNode.inl>
#include <noggit/ui/tools/NodeEditor/Nodes/DataTypes/GenericData.hpp>
#include <external/NodeEditor/include/nodes/Node>

#include <QImage>
#include <QColor>
#include <glm/glm.hpp>
#include <algorithm>
#include <cmath>

using namespace Noggit::Ui::Tools::NodeEditor::Nodes;

ImageGenerateLightmapNode::ImageGenerateLightmapNode()
: LogicNodeBase()
{
  setName("Image :: GenerateLightmap");
  setCaption("Image :: GenerateLightmap");
  _validation_state = NodeValidationState::Valid;

  addPortDefault<LogicData>(PortType::In, "Logic", true);
  addPortDefault<ImageData>(PortType::In, "Heightmap", true);
  addPortDefault<DecimalData>(PortType::In, "SunAzimuth", true);
  addPortDefault<DecimalData>(PortType::In, "SunElevation", true);
  addPortDefault<DecimalData>(PortType::In, "Ambient", true);
  addPortDefault<DecimalData>(PortType::In, "Intensity", true);
  addPortDefault<DecimalData>(PortType::In, "Z-Scale", true);
  addPortDefault<DecimalData>(PortType::In, "Step", true);

  addPort<LogicData>(PortType::Out, "Logic", true);
  addPort<ImageData>(PortType::Out, "Image", true);
}

void ImageGenerateLightmapNode::compute()
{
  QImage image = defaultPortData<ImageData>(PortType::In, 1)->value();
  float sunAzimuth = static_cast<float>(defaultPortData<DecimalData>(PortType::In, 2)->value());
  float sunElevation = static_cast<float>(defaultPortData<DecimalData>(PortType::In, 3)->value());
  float ambient = static_cast<float>(defaultPortData<DecimalData>(PortType::In, 4)->value());
  float intensity = static_cast<float>(defaultPortData<DecimalData>(PortType::In, 5)->value());
  float z_scale = static_cast<float>(defaultPortData<DecimalData>(PortType::In, 6)->value());
  int step = static_cast<int>(defaultPortData<DecimalData>(PortType::In, 7)->value());

  if (step < 1) step = 1;

  QImage result(image.width(), image.height(), QImage::Format_RGB32);
  
  glm::vec3 lightDir(
      std::cos(sunElevation) * std::cos(sunAzimuth),
      std::sin(sunElevation),
      std::cos(sunElevation) * std::sin(sunAzimuth)
  );
  lightDir = glm::normalize(lightDir);

  float xy_scale = 1.0f / image.width();

  for (int y = 0; y < image.height(); ++y) {
      for (int x = 0; x < image.width(); ++x) {
          
          auto getH = [&](int px, int py) -> float {
              px = std::clamp(px, 0, image.width() - 1);
              py = std::clamp(py, 0, image.height() - 1);
              // Assume grayscale heightmap where r = g = b
              return qRed(image.pixel(px, py)) / 255.0f;
          };

          float hL = getH(x - step, y);
          float hR = getH(x + step, y);
          float hD = getH(x, y - step);
          float hU = getH(x, y + step);
          
          glm::vec3 normal(
              (hL - hR) * z_scale / (2.0f * step * xy_scale),
              1.0f,
              (hD - hU) * z_scale / (2.0f * step * xy_scale)
          );
          normal = glm::normalize(normal);
          
          float dotProduct = glm::dot(normal, lightDir);
          float lambert = std::max(0.0f, dotProduct);
          
          float finalLight = ambient + lambert * intensity;
          finalLight = std::clamp(finalLight, 0.0f, 1.0f);
          
          int colorVal = static_cast<int>(finalLight * 255.0f);
          result.setPixelColor(x, y, QColor(colorVal, colorVal, colorVal));
      }
  }

  _out_ports[0].out_value = std::make_shared<LogicData>(true);
  _node->onDataUpdated(0);

  _out_ports[1].out_value = std::make_shared<ImageData>(result);
  _node->onDataUpdated(1);
}

NodeValidationState ImageGenerateLightmapNode::validate()
{
  if (!static_cast<ImageData*>(_in_ports[1].in_value.lock().get()))
  {
    setValidationState(NodeValidationState::Error);
    setValidationMessage("Error: failed to evaluate image input.");
    return _validation_state;
  }
  
  if (!static_cast<DecimalData*>(_in_ports[2].in_value.lock().get()) ||
      !static_cast<DecimalData*>(_in_ports[3].in_value.lock().get()) ||
      !static_cast<DecimalData*>(_in_ports[4].in_value.lock().get()) ||
      !static_cast<DecimalData*>(_in_ports[5].in_value.lock().get()) ||
      !static_cast<DecimalData*>(_in_ports[6].in_value.lock().get()) ||
      !static_cast<DecimalData*>(_in_ports[7].in_value.lock().get()))
  {
    setValidationState(NodeValidationState::Error);
    setValidationMessage("Error: failed to evaluate parameter inputs.");
    return _validation_state;
  }

  return LogicNodeBase::validate();
}
