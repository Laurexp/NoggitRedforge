// This file is part of Noggit3, licensed under GNU General Public License (version 3).

#include "ScatterTool.hpp"

#include <noggit/ActionManager.hpp>
#include <noggit/Input.hpp>
#include <noggit/MapView.h>
#include <noggit/World.h>
#include <noggit/ui/tools/ToolPanel/ToolPanel.hpp>
#include <noggit/ui/tools/AssetBrowser/Ui/AssetBrowser.hpp>
#include <noggit/Misc.h>

#include <random>
#include <QDateTime>
#include <QTimer>
#include <QPointer>

namespace Noggit
{
    ScatterTool::ScatterTool(MapView* mapView)
        : Tool{ mapView }
    {
        _lastSpawnTime.start();
    }

    ScatterTool::~ScatterTool()
    {
        delete _scatterWidget;
    }

    char const* ScatterTool::name() const
    {
        return "Scatter Tool";
    }

    editing_mode ScatterTool::editingMode() const
    {
        return editing_mode::scatter;
    }

    Ui::FontNoggit::Icons ScatterTool::icon() const
    {
        return Ui::FontNoggit::TOOL_OBJECT_EDITOR; // TODO: Custom icon if needed
    }

    void ScatterTool::setupUi(Ui::Tools::ToolPanel* toolPanel)
    {
        _scatterWidget = new Noggit::Ui::ScatterToolWidget(mapView());
        toolPanel->registerTool(this, _scatterWidget);

        using AssetBrowser = Noggit::Ui::Tools::AssetBrowser::Ui::AssetBrowserWidget;
        
        // Fetch current selection
        std::string currentModel = mapView()->getAssetBrowserWidget()->getFilename();
        if (!currentModel.empty())
        {
            _scatterWidget->setSelectedModel(QString::fromStdString(currentModel));
        }

        QObject::connect(mapView()->getAssetBrowserWidget(), &AssetBrowser::selectionChanged, [this](std::string const& path) {
            _scatterWidget->setSelectedModel(QString::fromStdString(path));
        });
    }

    ToolDrawParameters ScatterTool::drawParameters() const
    {
        return ToolDrawParameters
        {
            .radius = _scatterWidget ? _scatterWidget->getBrushRadius() : 10.0f,
            .inner_radius = 0.0f,
            .cursor_type = CursorType::CIRCLE,
            .terrain_type = eTerrainType_Flat,
        };
    }

    void ScatterTool::onSelected()
    {
        // Any setup when tool is activated
    }

    void ScatterTool::onTick(float deltaTime, TickParameters const& params)
    {
        if (!params.left_mouse || !_scatterWidget)
            return;

        auto models = _scatterWidget->getSelectedModels();
        if (models.empty())
            return;

        // Rate limit spawning based on density
        int density = _scatterWidget->getDensity();
        int spawnIntervalMs = 1000 / qMax(1, density * 2); // roughly density * 2 models per sec if dragging continuously

        if (_lastSpawnTime.elapsed() < spawnIntervalMs)
            return;

        _lastSpawnTime.restart();

        auto mv = mapView();
        auto world = mv->getWorld();

        float brushRadius = _scatterWidget->getBrushRadius();
        
        unsigned int ms = static_cast<unsigned>(QDateTime::currentMSecsSinceEpoch());
        std::mt19937 gen(ms);
        std::uniform_real_distribution<float> angleDist(0.0f, glm::two_pi<float>());
        std::uniform_real_distribution<float> radiusDist(0.0f, brushRadius);
        
        float angle = angleDist(gen);
        float r = radiusDist(gen);
        
        glm::vec3 centerPos = mv->cursorPosition();
        glm::vec3 spawnPos = centerPos;
        spawnPos.x += std::cos(angle) * r;
        spawnPos.z += std::sin(angle) * r; // Note: In WoW coords, Y is usually up, but Noggit sometimes uses Z. Wait, get_ground_height takes (X, Y, Z) and returns height on Y. Actually, get_ground_height returns the height on the Y axis in Noggit's internal coordinate system.

        float groundHeight = world->get_ground_height(spawnPos).y;
        
        // Ensure the spawn position is placed on the ground
        spawnPos.y = groundHeight;

        // Check height constraints
        float minHeight = _scatterWidget->getMinHeight();
        float maxHeight = _scatterWidget->getMaxHeight();

        if (spawnPos.y < minHeight || spawnPos.y > maxHeight)
            return;

        float minDistance = _scatterWidget->getMinDistance();

        // Check distance against objects spawned during this stroke (fast local check)
        for (const auto& p : _spawnedInStroke) {
            if (glm::distance(spawnPos, p) < minDistance) {
                return; // Too close to another object placed just now
            }
        }

        // Check distance against objects already existing in the world spatial grid
        auto nearby = world->getObjectsGroundDistance(spawnPos, minDistance, true, true);
        if (!nearby.empty()) {
            return; // Too close to an existing object
        }

        // Generate Scale
        std::uniform_real_distribution<float> scaleDist(_scatterWidget->getMinScale(), _scatterWidget->getMaxScale());
        float scale = scaleDist(gen);

        // Generate Rotation
        math::degrees::vec3 rotation(0.0f, 0.0f, 0.0f);
        if (_scatterWidget->getRandomRotation())
        {
            std::uniform_real_distribution<float> rotZDist(0.0f, 360.0f);
            rotation.y = rotZDist(gen); // Noggit models rotate around Y for heading
        }

        // Pick a random model from the selected list
        std::uniform_int_distribution<size_t> modelDist(0, models.size() - 1);
        QString modelStr = models[modelDist(gen)];

        std::string modelPath = modelStr.toStdString();
        bool isWMO = QString::fromStdString(modelPath).endsWith(".wmo", Qt::CaseInsensitive);

        if (isWMO)
        {
            auto wmo = world->addWMOAndGetInstance(modelPath, spawnPos, rotation, scale, true);
            wmo->wmo->wait_until_loaded();
            wmo->wmo->waitForChildrenLoaded();
            wmo->recalcExtents();
        }
        else
        {
            auto m2 = world->addM2AndGetInstance(modelPath, spawnPos, scale, rotation, nullptr, true, true);
            m2->model->wait_until_loaded();
            m2->model->waitForChildrenLoaded();
            m2->recalcExtents();
        }
        _spawnedInStroke.push_back(spawnPos);
        world->need_model_updates = true;
    }

    void ScatterTool::onMousePress(MousePressParameters const& params)
    {
        _spawnedInStroke.clear();
        _lastMousePos = params.mouse_position;

        if (params.mod_shift_down)
        {
            return; // Don't scatter if Shift was held
        }

        if (params.button == Qt::LeftButton)
            NOGGIT_ACTION_MGR->beginAction(mapView(), Noggit::ActionFlags::eOBJECTS_ADDED);
    }

    void ScatterTool::onMouseRelease(MouseReleaseParameters const& params)
    {
        if (params.button == Qt::LeftButton)
            NOGGIT_ACTION_MGR->endAction();
    }

    void ScatterTool::onMouseMove(MouseMoveParameters const& params)
    {
        _lastMousePos = params.mouse_position;
    }
}
