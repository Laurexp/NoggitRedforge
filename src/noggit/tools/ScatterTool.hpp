// This file is part of Noggit3, licensed under GNU General Public License (version 3).

#pragma once

#include <noggit/Tool.hpp>
#include <noggit/ui/ScatterToolWidget.hpp>
#include <QElapsedTimer>
#include <QPointF>

namespace Noggit
{
    class ScatterTool : public Tool
    {
    public:
        explicit ScatterTool(MapView* mapView);
        ~ScatterTool() override;

        char const* name() const override;
        editing_mode editingMode() const override;
        Ui::FontNoggit::Icons icon() const override;

        void setupUi(Ui::Tools::ToolPanel* toolPanel) override;
        ToolDrawParameters drawParameters() const override;
        
        void onSelected() override;
        void onTick(float deltaTime, TickParameters const& params) override;
        void onMouseMove(MouseMoveParameters const& params) override;
        void onMousePress(MousePressParameters const& params) override;
        void onMouseRelease(MouseReleaseParameters const& params) override;

    private:
        Noggit::Ui::ScatterToolWidget* _scatterWidget = nullptr;
        QElapsedTimer _lastSpawnTime;
        QPointF _lastMousePos;
        std::vector<glm::vec3> _spawnedInStroke;
    };
}
