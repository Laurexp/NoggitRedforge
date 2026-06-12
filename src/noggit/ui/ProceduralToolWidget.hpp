#pragma once

#include <QDialog>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QComboBox>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QScrollArea>
#include <QTabWidget>
#include <QPainter>
#include <QMouseEvent>
#include <vector>
#include <noggit/ui/CurrentTexture.h>

class MapView;
class World;

namespace Noggit::Ui
{
    struct LayerUI {
        QGroupBox* group;
        QComboBox* type;
        QComboBox* blend;
        QSpinBox* seed;
        QSpinBox* octaves;
        QDoubleSpinBox* scale;
        QDoubleSpinBox* persistence;
        QDoubleSpinBox* frequency;
        QDoubleSpinBox* weight;
        QPushButton* removeBtn;
    };

    class GridSelectionWidget : public QWidget {
        Q_OBJECT
    public:
        explicit GridSelectionWidget(QWidget* parent = nullptr);
        void setMapBounds(int minX, int maxX, int minY, int maxY);
        void setWorld(World* world);
        void setSelectionVector(std::vector<char>* selection);

    signals:
        void selectionChanged();

    protected:
        void paintEvent(QPaintEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;

    private:
        int _mapMinX = 0, _mapMaxX = 63, _mapMinY = 0, _mapMaxY = 63;
        bool _isDragging = false;
        bool _dragToggleState = false;
        World* _world = nullptr;
        std::vector<char>* _selection = nullptr;
        
        void toggleTile(QMouseEvent* event);
    };

    class SunDirectionWidget : public QWidget {
        Q_OBJECT
    public:
        explicit SunDirectionWidget(QWidget* parent = nullptr);
        void setDirection(float azimuth, float elevation);
        float azimuth() const { return _azimuth; }
        float elevation() const { return _elevation; }

    signals:
        void directionChanged(float azimuth, float elevation);

    protected:
        void paintEvent(QPaintEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;

    private:
        float _azimuth = 0.0f; // 0-360
        float _elevation = 45.0f; // 0-90
        bool _isDragging = false;
        void updateFromMouse(QMouseEvent* event);
    };

    class ProceduralToolWidget : public QDialog
    {
        Q_OBJECT
    public:
        ProceduralToolWidget(QWidget* parent = nullptr, MapView* mapView = nullptr);
        void setMapView(MapView* mapView) { _mapView = mapView; }

    protected:
        void showEvent(QShowEvent* event) override;

    private slots:
        void onApply();
        void onAddLayer();
        void onClearLayers();
        void onRandomizeSeeds();
        void onLoadPreset();
        void onLoadJson();

        void onSavePalette();
        void onLoadPalette();
        void onCapturePalette();
        void onDeletePalette();
        void reloadPalettes();

        void onGenerateLightmap();
        void onTestLightmap();
        void onClearLightmap();
        void onPresetChanged(int index);
        void onSaveLmPreset();
        void onDeleteLmPreset();
        void reloadLmPresets();

    private:
        void addLayerUI(int typeIdx, int blendIdx, int seed, int octaves, float scale, float persistence, float frequency, float weight);

        MapView* _mapView;

        // Presets
        QComboBox* _presetCombo;
        QPushButton* _loadPresetBtn;
        QPushButton* _loadJsonBtn;

        // Layers
        QScrollArea* _scrollArea;
        QWidget* _scrollWidget;
        QVBoxLayout* _layersLayout;
        std::vector<LayerUI> _layers;
        QPushButton* _addLayerBtn;
        QPushButton* _clearLayersBtn;
        QPushButton* _randomizeSeedsBtn;

        // Modifiers
        QSpinBox* _terraceSteps;
        QSpinBox* _smoothingRadius;
        QDoubleSpinBox* _islandMix;
        QDoubleSpinBox* _exponent;

        // Erosion
        QSpinBox* _erosionIterations;

        // Target Area
        QGroupBox* _targetGroup;
        QCheckBox* _useTargetArea;
        QTabWidget* _targetTabs;
        GridSelectionWidget* _gridWidget;
        std::vector<char> _proceduralSelection;
        QLabel* _selectionInfoLabel;
        QSpinBox* _targetMinX;
        QSpinBox* _targetMaxX;
        QSpinBox* _targetMinY;
        QSpinBox* _targetMaxY;

        // Palette Manager
        QComboBox* _paletteCombo;
        QPushButton* _loadPaletteBtn;
        QPushButton* _savePaletteBtn;
        QPushButton* _capturePaletteBtn;
        QPushButton* _deletePaletteBtn;

        // Textures
        Noggit::Ui::current_texture* _texBase;
        Noggit::Ui::current_texture* _texRock;
        Noggit::Ui::current_texture* _texRiver;
        Noggit::Ui::current_texture* _texExtra;

        QPushButton* _applyBtn;

        // Lightmap / Vertex Color
        QGroupBox* _lightmapGroup;
        SunDirectionWidget* _sunWidget;
        QComboBox* _lmPresetCombo;
        QPushButton* _saveLmPresetBtn;
        QPushButton* _deleteLmPresetBtn;
        QDoubleSpinBox* _lmAzimuth;
        QDoubleSpinBox* _lmElevation;
        QDoubleSpinBox* _lmAmbient;
        QDoubleSpinBox* _lmIntensity;
        QDoubleSpinBox* _lmZScale;
        QSpinBox* _lmStep;
        QPushButton* _generateLightmapBtn;
        QPushButton* _testLightmapBtn;
        QPushButton* _clearLightmapBtn;
    };
}
