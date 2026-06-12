#include <noggit/ui/ProceduralToolWidget.hpp>
#include <noggit/Log.h>
#include <noggit/MapView.h>
#include <noggit/MapChunk.h>
#include <noggit/MapTile.h>
#include <noggit/texture_set.hpp>
#include <noggit/World.h>
#include <noggit/Camera.hpp>

#include "../procedural/erosion/TerrainTypes.h"
#include "../procedural/erosion/HydraulicErosion.h"
#include "../procedural/erosion/Hydrology.h"
#include "../procedural/noise/HeightmapGenerator.h"
#include "../procedural/utils/Filters.h"

#include <QGroupBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QSettings>
#include <QInputDialog>
#include <nlohmann/json.hpp>
#include <fstream>
#include <QFileDialog>
#include <QToolTip>
#include <noggit/ui/minimap_widget.hpp>
#include <noggit/World.h>

namespace Noggit::Ui
{
    GridSelectionWidget::GridSelectionWidget(QWidget* parent) : QWidget(parent) {
        setMinimumSize(256, 256);
        setCursor(Qt::CrossCursor);
    }

    void GridSelectionWidget::setMapBounds(int minX, int maxX, int minY, int maxY) {
        _mapMinX = minX; _mapMaxX = maxX; _mapMinY = minY; _mapMaxY = maxY;
        update();
    }

    void GridSelectionWidget::setWorld(World* world) {
        _world = world;
        update();
    }

    void GridSelectionWidget::setSelectionVector(std::vector<char>* selection) {
        _selection = selection;
        update();
    }

    void GridSelectionWidget::paintEvent(QPaintEvent* event) {
        QPainter p(this);
        p.fillRect(rect(), QColor(30, 30, 30));
        
        int gridW = std::max(1, _mapMaxX - _mapMinX + 1);
        int gridH = std::max(1, _mapMaxY - _mapMinY + 1);
        
        float cellW = width() / (float)gridW;
        float cellH = height() / (float)gridH;

        if (_world && _selection) {
            for (int y = _mapMinY; y <= _mapMaxY; ++y) {
                for (int x = _mapMinX; x <= _mapMaxX; ++x) {
                    bool hasTile = _world->mapIndex.hasTile(TileIndex(x, y)) || _world->mapIndex.getTile(TileIndex(x, y)) != nullptr;
                    bool selected = _selection->at(x * 64 + y);
                    
                    QRectF cellRect((x - _mapMinX) * cellW, (y - _mapMinY) * cellH, cellW, cellH);
                    
                    if (hasTile) {
                        if (selected) {
                            p.fillRect(cellRect, QColor(0, 150, 255));
                        } else {
                            p.fillRect(cellRect, QColor(80, 80, 80));
                        }
                    } else {
                        // Empty space
                        p.fillRect(cellRect, QColor(40, 40, 40));
                    }
                }
            }
        }

        p.setPen(QColor(60, 60, 60));
        for (int i = 0; i <= gridH; ++i) { p.drawLine(0, i * cellH, width(), i * cellH); }
        for (int i = 0; i <= gridW; ++i) { p.drawLine(i * cellW, 0, i * cellW, height()); }
    }

    void GridSelectionWidget::toggleTile(QMouseEvent* event) {
        if (!_world || !_selection) return;
        
        int gridW = std::max(1, _mapMaxX - _mapMinX + 1);
        int gridH = std::max(1, _mapMaxY - _mapMinY + 1);
        float cellW = width() / (float)gridW;
        float cellH = height() / (float)gridH;
        
        int tx = std::clamp((int)(event->pos().x() / cellW) + _mapMinX, _mapMinX, _mapMaxX);
        int ty = std::clamp((int)(event->pos().y() / cellH) + _mapMinY, _mapMinY, _mapMaxY);
        
        if (_world->mapIndex.hasTile(TileIndex(tx, ty)) || _world->mapIndex.getTile(TileIndex(tx, ty)) != nullptr) {
            (*_selection)[tx * 64 + ty] = _dragToggleState;
            update();
            emit selectionChanged();
        }
    }

    void GridSelectionWidget::mousePressEvent(QMouseEvent* event) {
        if (event->button() == Qt::LeftButton) {
            _isDragging = true;
            int gridW = std::max(1, _mapMaxX - _mapMinX + 1);
            int gridH = std::max(1, _mapMaxY - _mapMinY + 1);
            float cellW = width() / (float)gridW;
            float cellH = height() / (float)gridH;
            int tx = std::clamp((int)(event->pos().x() / cellW) + _mapMinX, _mapMinX, _mapMaxX);
            int ty = std::clamp((int)(event->pos().y() / cellH) + _mapMinY, _mapMinY, _mapMaxY);
            
            if (_world && _selection && (_world->mapIndex.hasTile(TileIndex(tx, ty)) || _world->mapIndex.getTile(TileIndex(tx, ty)) != nullptr)) {
                _dragToggleState = !(*_selection)[tx * 64 + ty];
                toggleTile(event);
            }
        }
    }

    void GridSelectionWidget::mouseMoveEvent(QMouseEvent* event) {
        if (_isDragging) {
            toggleTile(event);
        }
    }

    void GridSelectionWidget::mouseReleaseEvent(QMouseEvent* event) {
        if (event->button() == Qt::LeftButton) {
            _isDragging = false;
        }
    }

    SunDirectionWidget::SunDirectionWidget(QWidget* parent) : QWidget(parent) {
        setMinimumSize(100, 100);
        setMaximumSize(150, 150);
        setCursor(Qt::CrossCursor);
    }

    void SunDirectionWidget::setDirection(float azimuth, float elevation) {
        if (_azimuth == azimuth && _elevation == elevation) return;
        _azimuth = azimuth;
        _elevation = elevation;
        update();
    }

    void SunDirectionWidget::paintEvent(QPaintEvent* event) {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        int cx = width() / 2;
        int cy = height() / 2;
        int radius = std::min(cx, cy) - 4;

        // Background circle
        p.setBrush(QColor(40, 40, 40));
        p.setPen(QColor(100, 100, 100));
        p.drawEllipse(cx - radius, cy - radius, radius * 2, radius * 2);

        // Grid lines
        p.setPen(QColor(80, 80, 80));
        p.drawLine(cx - radius, cy, cx + radius, cy);
        p.drawLine(cx, cy - radius, cx, cy + radius);
        p.drawEllipse(cx - radius / 2, cy - radius / 2, radius, radius);

        // Calculate sun position
        // Azimuth: 0 = North (Up), 90 = East (Right), 180 = South (Down), 270 = West (Left)
        // Elevation: 0 = Edge of circle, 90 = Center
        float r = radius * (1.0f - (_elevation / 90.0f));
        float azRad = (_azimuth - 90.0f) * (3.1415926535f / 180.0f); // -90 because 0 is Up
        int sx = cx + r * std::cos(azRad);
        int sy = cy + r * std::sin(azRad);

        // Draw sun point
        p.setBrush(QColor(255, 200, 0));
        p.setPen(Qt::NoPen);
        p.drawEllipse(sx - 5, sy - 5, 10, 10);
    }

    void SunDirectionWidget::updateFromMouse(QMouseEvent* event) {
        int cx = width() / 2;
        int cy = height() / 2;
        int radius = std::min(cx, cy) - 4;

        float dx = event->pos().x() - cx;
        float dy = event->pos().y() - cy;

        float dist = std::sqrt(dx * dx + dy * dy);
        float r = std::clamp(dist, 0.0f, (float)radius);

        _elevation = 90.0f * (1.0f - (r / radius));

        float angle = std::atan2(dy, dx) * (180.0f / 3.1415926535f);
        _azimuth = angle + 90.0f;
        if (_azimuth < 0.0f) _azimuth += 360.0f;
        if (_azimuth >= 360.0f) _azimuth -= 360.0f;

        update();
        emit directionChanged(_azimuth, _elevation);
    }

    void SunDirectionWidget::mousePressEvent(QMouseEvent* event) {
        if (event->button() == Qt::LeftButton) {
            _isDragging = true;
            updateFromMouse(event);
        }
    }

    void SunDirectionWidget::mouseMoveEvent(QMouseEvent* event) {
        if (_isDragging) {
            updateFromMouse(event);
        }
    }

    ProceduralToolWidget::ProceduralToolWidget(QWidget* parent, MapView* mapView)
        : QDialog(parent), _mapView(mapView), _proceduralSelection(64 * 64, 0)
    {
        setWindowTitle("Procedural Terrain Generation");
        auto mainLayout = new QVBoxLayout(this);
        
        auto mainScroll = new QScrollArea(this);
        mainScroll->setWidgetResizable(true);
        mainScroll->setFrameShape(QFrame::NoFrame);
        auto scrollContent = new QWidget(mainScroll);
        auto layout = new QVBoxLayout(scrollContent);
        mainScroll->setWidget(scrollContent);
        mainLayout->addWidget(mainScroll);
        
        // Presets
        auto presetGroup = new QGroupBox("Presets");
        auto presetLayout = new QHBoxLayout(presetGroup);
        _presetCombo = new QComboBox(this);
        _presetCombo->addItems({"Default", "Mountain Range", "WoWNoggit", "Islas", "Crateres", "Picos y Valles", "Costa Montanosa"});
        presetLayout->addWidget(_presetCombo);
        _loadPresetBtn = new QPushButton("Load Preset", this);
        presetLayout->addWidget(_loadPresetBtn);
        _loadJsonBtn = new QPushButton("Load from JSON...", this);
        presetLayout->addWidget(_loadJsonBtn);
        layout->addWidget(presetGroup);
        connect(_loadPresetBtn, &QPushButton::clicked, this, &ProceduralToolWidget::onLoadPreset);
        connect(_loadJsonBtn, &QPushButton::clicked, this, &ProceduralToolWidget::onLoadJson);

        // Layers Area
        _scrollWidget = new QWidget(this);
        _layersLayout = new QVBoxLayout(_scrollWidget);
        layout->addWidget(_scrollWidget);

        auto layerBtns = new QHBoxLayout();
        _addLayerBtn = new QPushButton("Add Layer", this);
        _clearLayersBtn = new QPushButton("Clear Layers", this);
        _randomizeSeedsBtn = new QPushButton("Randomize Seeds", this);
        layerBtns->addWidget(_addLayerBtn);
        layerBtns->addWidget(_clearLayersBtn);
        layerBtns->addWidget(_randomizeSeedsBtn);
        layout->addLayout(layerBtns);
        
        connect(_addLayerBtn, &QPushButton::clicked, this, &ProceduralToolWidget::onAddLayer);
        connect(_clearLayersBtn, &QPushButton::clicked, this, &ProceduralToolWidget::onClearLayers);
        connect(_randomizeSeedsBtn, &QPushButton::clicked, this, &ProceduralToolWidget::onRandomizeSeeds);

        // Modifiers
        auto modGroup = new QGroupBox("Modifiers");
        auto modForm = new QFormLayout(modGroup);
        _terraceSteps = new QSpinBox(this); _terraceSteps->setRange(0, 50); _terraceSteps->setValue(0);
        modForm->addRow("Terrace Steps:", _terraceSteps);
        _smoothingRadius = new QSpinBox(this); _smoothingRadius->setRange(0, 10); _smoothingRadius->setValue(0);
        modForm->addRow("Smoothing Radius:", _smoothingRadius);
        _islandMix = new QDoubleSpinBox(this); _islandMix->setRange(0.0, 1.0); _islandMix->setValue(0.0); _islandMix->setSingleStep(0.1);
        modForm->addRow("Island Mix:", _islandMix);
        _exponent = new QDoubleSpinBox(this); _exponent->setRange(0.1, 5.0); _exponent->setValue(1.0); _exponent->setSingleStep(0.1);
        modForm->addRow("Exponent:", _exponent);
        layout->addWidget(modGroup);

        // Erosion
        auto erosionGroup = new QGroupBox("Hydraulic Erosion");
        auto erosionForm = new QFormLayout(erosionGroup);
        _erosionIterations = new QSpinBox(this);
        _erosionIterations->setRange(0, 5000000);
        _erosionIterations->setValue(100000);
        erosionForm->addRow("Droplets:", _erosionIterations);
        layout->addWidget(erosionGroup);
        
        // Target Area
        _targetGroup = new QGroupBox("Target Area (Grid ADT)");
        auto targetForm = new QVBoxLayout(_targetGroup);

        auto enableLayout = new QHBoxLayout();
        _useTargetArea = new QCheckBox("Enable Area Limit", this);
        _useTargetArea->setChecked(false);
        enableLayout->addWidget(_useTargetArea);
        
        // Tooltip button
        auto tooltipBtn = new QPushButton("?", this);
        tooltipBtn->setFixedSize(20, 20);
        tooltipBtn->setToolTip("Activa esta opción para limitar la generación procedural a un área específica del mapa.\nPuedes arrastrar el ratón en la pestaña 'Visual' o usar valores exactos en 'Technical'.");
        enableLayout->addWidget(tooltipBtn);
        enableLayout->addStretch();
        targetForm->addLayout(enableLayout);

        _targetTabs = new QTabWidget(this);

        // Visual Tab
        auto visualTab = new QWidget();
        auto visualLayout = new QVBoxLayout(visualTab);
        _gridWidget = new GridSelectionWidget(this);
        visualLayout->addWidget(_gridWidget);
        _selectionInfoLabel = new QLabel("Selected ADT: -", this);
        _selectionInfoLabel->setAlignment(Qt::AlignCenter);
        visualLayout->addWidget(_selectionInfoLabel);
        _targetTabs->addTab(visualTab, "Visual");

        // Technical Tab
        auto techTab = new QWidget();
        auto techForm = new QFormLayout(techTab);
        auto xLayout = new QHBoxLayout();
        _targetMinX = new QSpinBox(this); _targetMinX->setRange(0, 63); _targetMinX->setValue(0);
        _targetMaxX = new QSpinBox(this); _targetMaxX->setRange(0, 63); _targetMaxX->setValue(63);
        xLayout->addWidget(new QLabel("Min X:")); xLayout->addWidget(_targetMinX);
        xLayout->addWidget(new QLabel("Max X:")); xLayout->addWidget(_targetMaxX);
        techForm->addRow("X Range:", xLayout);

        auto yLayout = new QHBoxLayout();
        _targetMinY = new QSpinBox(this); _targetMinY->setRange(0, 63); _targetMinY->setValue(0);
        _targetMaxY = new QSpinBox(this); _targetMaxY->setRange(0, 63); _targetMaxY->setValue(63);
        yLayout->addWidget(new QLabel("Min Y:")); yLayout->addWidget(_targetMinY);
        yLayout->addWidget(new QLabel("Max Y:")); yLayout->addWidget(_targetMaxY);
        techForm->addRow("Y Range:", yLayout);
        _targetTabs->addTab(techTab, "Technical");

        targetForm->addWidget(_targetTabs);
        layout->addWidget(_targetGroup);

        // Connect Grid tile clicked to update Tech tab bounds
        connect(_gridWidget, &GridSelectionWidget::selectionChanged, this, [this]() {
            int minX = 64, maxX = -1, minY = 64, maxY = -1;
            for (int y = 0; y < 64; ++y) {
                for (int x = 0; x < 64; ++x) {
                    if (_proceduralSelection[x * 64 + y]) {
                        minX = std::min(minX, x);
                        maxX = std::max(maxX, x);
                        minY = std::min(minY, y);
                        maxY = std::max(maxY, y);
                    }
                }
            }
            if (maxX >= 0) {
                _targetMinX->blockSignals(true); _targetMaxX->blockSignals(true);
                _targetMinY->blockSignals(true); _targetMaxY->blockSignals(true);
                _targetMinX->setValue(minX); _targetMaxX->setValue(maxX);
                _targetMinY->setValue(minY); _targetMaxY->setValue(maxY);
                _targetMinX->blockSignals(false); _targetMaxX->blockSignals(false);
                _targetMinY->blockSignals(false); _targetMaxY->blockSignals(false);
                _selectionInfoLabel->setText(QString("Selected ADTs: [%1, %2] to [%3, %4]").arg(minX).arg(minY).arg(maxX).arg(maxY));
            } else {
                _selectionInfoLabel->setText("No ADTs Selected");
            }
        });

        // Connect SpinBoxes to Grid
        auto updateGrid = [this]() {
            std::fill(_proceduralSelection.begin(), _proceduralSelection.end(), 0);
            for (int y = _targetMinY->value(); y <= _targetMaxY->value(); ++y) {
                for (int x = _targetMinX->value(); x <= _targetMaxX->value(); ++x) {
                    if (_mapView && _mapView->getWorld() && (_mapView->getWorld()->mapIndex.hasTile(TileIndex(x, y)) || _mapView->getWorld()->mapIndex.getTile(TileIndex(x, y)) != nullptr)) {
                        _proceduralSelection[x * 64 + y] = 1;
                    }
                }
            }
            _gridWidget->update();
            _selectionInfoLabel->setText(QString("Selected ADTs: [%1, %2] to [%3, %4]")
                .arg(_targetMinX->value()).arg(_targetMinY->value())
                .arg(_targetMaxX->value()).arg(_targetMaxY->value()));
        };
        connect(_targetMinX, QOverload<int>::of(&QSpinBox::valueChanged), this, updateGrid);
        connect(_targetMaxX, QOverload<int>::of(&QSpinBox::valueChanged), this, updateGrid);
        connect(_targetMinY, QOverload<int>::of(&QSpinBox::valueChanged), this, updateGrid);
        connect(_targetMaxY, QOverload<int>::of(&QSpinBox::valueChanged), this, updateGrid);
        
        // Palette Manager
        auto palGroup = new QGroupBox("Palette Manager");
        auto palLayout = new QHBoxLayout(palGroup);
        _paletteCombo = new QComboBox(this);
        palLayout->addWidget(_paletteCombo);
        
        _loadPaletteBtn = new QPushButton("Load", this);
        _savePaletteBtn = new QPushButton("Save", this);
        _deletePaletteBtn = new QPushButton("Delete", this);
        _capturePaletteBtn = new QPushButton("Capture from Map", this);
        
        palLayout->addWidget(_loadPaletteBtn);
        palLayout->addWidget(_savePaletteBtn);
        palLayout->addWidget(_deletePaletteBtn);
        palLayout->addWidget(_capturePaletteBtn);
        layout->addWidget(palGroup);
        
        connect(_loadPaletteBtn, &QPushButton::clicked, this, &ProceduralToolWidget::onLoadPalette);
        connect(_savePaletteBtn, &QPushButton::clicked, this, &ProceduralToolWidget::onSavePalette);
        connect(_deletePaletteBtn, &QPushButton::clicked, this, &ProceduralToolWidget::onDeletePalette);
        connect(_capturePaletteBtn, &QPushButton::clicked, this, &ProceduralToolWidget::onCapturePalette);

        // Textures
        auto texGroup = new QGroupBox("Autopaint Textures");
        auto texLayout = new QHBoxLayout(texGroup);
        
        auto vboxBase = new QVBoxLayout();
        vboxBase->addWidget(new QLabel("Base (Flat)"));
        _texBase = new Noggit::Ui::current_texture(true, this);
        _texBase->set_texture("Tileset\\Generic\\Dirt.blp");
        vboxBase->addWidget(_texBase);
        texLayout->addLayout(vboxBase);

        auto vboxRock = new QVBoxLayout();
        vboxRock->addWidget(new QLabel("Rock (Steep)"));
        _texRock = new Noggit::Ui::current_texture(true, this);
        _texRock->set_texture("Tileset\\Generic\\Rock.blp");
        vboxRock->addWidget(_texRock);
        texLayout->addLayout(vboxRock);

        auto vboxRiver = new QVBoxLayout();
        vboxRiver->addWidget(new QLabel("River (Mud)"));
        _texRiver = new Noggit::Ui::current_texture(true, this);
        _texRiver->set_texture("Tileset\\Generic\\Mud.blp");
        vboxRiver->addWidget(_texRiver);
        texLayout->addLayout(vboxRiver);
        
        auto vboxExtra = new QVBoxLayout();
        vboxExtra->addWidget(new QLabel("Extra (4th)"));
        _texExtra = new Noggit::Ui::current_texture(true, this);
        _texExtra->set_texture("Tileset\\Generic\\Snow.blp");
        vboxExtra->addWidget(_texExtra);
        texLayout->addLayout(vboxExtra);
        
        layout->addWidget(texGroup);

        // Vertex Color Lightmap
        _lightmapGroup = new QGroupBox("Vertex Color Lightmap");
        auto lmLayout = new QHBoxLayout(_lightmapGroup);

        _sunWidget = new SunDirectionWidget(this);
        lmLayout->addWidget(_sunWidget);

        auto lmFormLayout = new QFormLayout();

        auto lmPresetLayout = new QHBoxLayout();
        _lmPresetCombo = new QComboBox(this);
        _saveLmPresetBtn = new QPushButton("Save", this);
        _deleteLmPresetBtn = new QPushButton("Del", this);
        lmPresetLayout->addWidget(_lmPresetCombo);
        lmPresetLayout->addWidget(_saveLmPresetBtn);
        lmPresetLayout->addWidget(_deleteLmPresetBtn);
        lmFormLayout->addRow("Preset:", lmPresetLayout);
        reloadLmPresets();

        _lmAzimuth = new QDoubleSpinBox(this); _lmAzimuth->setRange(0.0, 360.0); _lmAzimuth->setValue(0.0); _lmAzimuth->setSingleStep(5.0);
        _lmAzimuth->setToolTip("Azimuth: El ángulo de donde viene el sol (0º = Norte, 90º = Este, 180º = Sur, 270º = Oeste).");
        lmFormLayout->addRow("Sun Azimuth:", _lmAzimuth);

        _lmElevation = new QDoubleSpinBox(this); _lmElevation->setRange(0.0, 90.0); _lmElevation->setValue(45.0); _lmElevation->setSingleStep(5.0);
        _lmElevation->setToolTip("Elevation: La altura del sol. 0º = Amanecer/Atardecer (sombras largas), 90º = Mediodía (sombras directas hacia abajo).");
        lmFormLayout->addRow("Sun Elevation:", _lmElevation);

        _lmAmbient = new QDoubleSpinBox(this); _lmAmbient->setRange(0.0, 2.0); _lmAmbient->setValue(0.2); _lmAmbient->setSingleStep(0.05);
        _lmAmbient->setToolTip("Ambient: Luz base para zonas a la sombra. Un valor de 0 hace sombras completamente negras. 1.0 es el gris neutro por defecto de WoW.");
        lmFormLayout->addRow("Ambient:", _lmAmbient);

        _lmIntensity = new QDoubleSpinBox(this); _lmIntensity->setRange(0.0, 5.0); _lmIntensity->setValue(1.0); _lmIntensity->setSingleStep(0.1);
        _lmIntensity->setToolTip("Intensity: Intensidad de la luz solar directa. Define cuán blanca se verá una cara impactada por la luz.");
        lmFormLayout->addRow("Intensity:", _lmIntensity);

        _lmZScale = new QDoubleSpinBox(this); _lmZScale->setRange(0.1, 1000.0); _lmZScale->setValue(100.0); _lmZScale->setSingleStep(10.0);
        _lmZScale->setToolTip("Z-Scale: Exagera artificialmente la altura para el cálculo de luz. Mayor escala = sombras más marcadas en montañas.");
        lmFormLayout->addRow("Z-Scale:", _lmZScale);

        _lmStep = new QSpinBox(this); _lmStep->setRange(1, 20); _lmStep->setValue(5);
        _lmStep->setToolTip("Step: Precisión del cálculo. Usa 1 para vértices exactos (puede crear artefactos escalonados en relieves editados). Usa >3 para sombras más suaves y difuminadas.");
        lmFormLayout->addRow("Step:", _lmStep);

        auto btnLayout = new QHBoxLayout();
        _generateLightmapBtn = new QPushButton("Generate", this);
        _testLightmapBtn = new QPushButton("Test", this);
        _clearLightmapBtn = new QPushButton("Clear", this);
        btnLayout->addWidget(_generateLightmapBtn);
        btnLayout->addWidget(_testLightmapBtn);
        btnLayout->addWidget(_clearLightmapBtn);
        lmFormLayout->addRow(btnLayout);

        lmLayout->addLayout(lmFormLayout);

        layout->addWidget(_lightmapGroup);
        connect(_lmPresetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ProceduralToolWidget::onPresetChanged);
        connect(_saveLmPresetBtn, &QPushButton::clicked, this, &ProceduralToolWidget::onSaveLmPreset);
        connect(_deleteLmPresetBtn, &QPushButton::clicked, this, &ProceduralToolWidget::onDeleteLmPreset);
        connect(_generateLightmapBtn, &QPushButton::clicked, this, &ProceduralToolWidget::onGenerateLightmap);
        connect(_testLightmapBtn, &QPushButton::clicked, this, &ProceduralToolWidget::onTestLightmap);
        connect(_clearLightmapBtn, &QPushButton::clicked, this, &ProceduralToolWidget::onClearLightmap);

        // Connect Sun Widget and SpinBoxes bidirectionally
        connect(_sunWidget, &SunDirectionWidget::directionChanged, this, [this](float az, float el) {
            _lmAzimuth->blockSignals(true);
            _lmElevation->blockSignals(true);
            _lmAzimuth->setValue(az);
            _lmElevation->setValue(el);
            _lmAzimuth->blockSignals(false);
            _lmElevation->blockSignals(false);
        });
        auto updateSunWidget = [this]() {
            _sunWidget->setDirection(_lmAzimuth->value(), _lmElevation->value());
        };
        connect(_lmAzimuth, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, updateSunWidget);
        connect(_lmElevation, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, updateSunWidget);
        updateSunWidget(); // init state

        _applyBtn = new QPushButton("Generate Terrain", this);
        mainLayout->addWidget(_applyBtn);
        
        connect(_applyBtn, &QPushButton::clicked, this, &ProceduralToolWidget::onApply);

        // Add default layer
        addLayerUI(1, 0, 1337, 4, 2.0f, 0.5f, 0.005f, 1.0f); // Simplex, Add
        
        reloadPalettes();
    }

    void ProceduralToolWidget::showEvent(QShowEvent* event) {
        QDialog::showEvent(event);
        if (_mapView && _gridWidget) {
            _gridWidget->setWorld(_mapView->getWorld());
            _gridWidget->setSelectionVector(&_proceduralSelection);
            
            int mapMinX = 64, mapMaxX = -1, mapMinY = 64, mapMaxY = -1;
            if (_mapView->getWorld()) {
                for (int ty = 0; ty < 64; ++ty) {
                    for (int tx = 0; tx < 64; ++tx) {
                        if (_mapView->getWorld()->mapIndex.hasTile(TileIndex(tx, ty)) || _mapView->getWorld()->mapIndex.getTile(TileIndex(tx, ty)) != nullptr) {
                            mapMinX = std::min(mapMinX, tx);
                            mapMaxX = std::max(mapMaxX, tx);
                            mapMinY = std::min(mapMinY, ty);
                            mapMaxY = std::max(mapMaxY, ty);
                        }
                    }
                }
            }
            if (mapMaxX < 0) { mapMinX = 0; mapMaxX = 63; mapMinY = 0; mapMaxY = 63; }
            _gridWidget->setMapBounds(mapMinX, mapMaxX, mapMinY, mapMaxY);

            // Initialize SpinBoxes and selection to map bounds
            _targetMinX->blockSignals(true); _targetMaxX->blockSignals(true);
            _targetMinY->blockSignals(true); _targetMaxY->blockSignals(true);
            _targetMinX->setValue(mapMinX); _targetMaxX->setValue(mapMaxX);
            _targetMinY->setValue(mapMinY); _targetMaxY->setValue(mapMaxY);
            _targetMinX->blockSignals(false); _targetMaxX->blockSignals(false);
            _targetMinY->blockSignals(false); _targetMaxY->blockSignals(false);

            std::fill(_proceduralSelection.begin(), _proceduralSelection.end(), 0);
            for (int y = mapMinY; y <= mapMaxY; ++y) {
                for (int x = mapMinX; x <= mapMaxX; ++x) {
                    if (_mapView->getWorld()->mapIndex.hasTile(TileIndex(x, y)) || _mapView->getWorld()->mapIndex.getTile(TileIndex(x, y)) != nullptr) {
                        _proceduralSelection[x * 64 + y] = 1;
                    }
                }
            }
            _selectionInfoLabel->setText(QString("Selected ADTs: [%1, %2] to [%3, %4]").arg(mapMinX).arg(mapMinY).arg(mapMaxX).arg(mapMaxY));
            _gridWidget->update();
        }
    }

    void ProceduralToolWidget::reloadPalettes() {
        _paletteCombo->clear();
        QSettings settings("Noggit", "ProceduralPalettes");
        _paletteCombo->addItems(settings.childGroups());
    }

    void ProceduralToolWidget::onSavePalette() {
        bool ok;
        QString text = QInputDialog::getText(this, "Save Palette",
                                             "Palette Name:", QLineEdit::Normal,
                                             "", &ok);
        if (ok && !text.isEmpty()) {
            QSettings settings("Noggit", "ProceduralPalettes");
            settings.beginGroup(text);
            settings.setValue("tex1", QString::fromStdString(_texBase->filename()));
            settings.setValue("tex2", QString::fromStdString(_texRock->filename()));
            settings.setValue("tex3", QString::fromStdString(_texRiver->filename()));
            settings.setValue("tex4", QString::fromStdString(_texExtra->filename()));
            settings.endGroup();
            reloadPalettes();
            _paletteCombo->setCurrentText(text);
        }
    }

    void ProceduralToolWidget::onLoadPalette() {
        QString current = _paletteCombo->currentText();
        if (current.isEmpty()) return;
        
        QSettings settings("Noggit", "ProceduralPalettes");
        settings.beginGroup(current);
        _texBase->set_texture(settings.value("tex1", "Tileset\\Generic\\Dirt.blp").toString().toStdString());
        _texRock->set_texture(settings.value("tex2", "Tileset\\Generic\\Rock.blp").toString().toStdString());
        _texRiver->set_texture(settings.value("tex3", "Tileset\\Generic\\Mud.blp").toString().toStdString());
        _texExtra->set_texture(settings.value("tex4", "Tileset\\Generic\\Snow.blp").toString().toStdString());
        settings.endGroup();
    }

    void ProceduralToolWidget::onDeletePalette() {
        QString current = _paletteCombo->currentText();
        if (current.isEmpty()) return;
        
        QSettings settings("Noggit", "ProceduralPalettes");
        settings.remove(current);
        reloadPalettes();
    }

    void ProceduralToolWidget::onCapturePalette() {
        if (!_mapView || !_mapView->getWorld() || !_mapView->getCamera()) return;
        
        MapChunk* chunk = _mapView->getWorld()->getChunkAt(_mapView->getCamera()->position);
        if (!chunk) {
            QMessageBox::warning(this, "No terrain", "Please point the camera at some terrain to capture its textures.");
            return;
        }
        

        if (chunk->texture_set) {
            if (chunk->texture_set->num() > 0) _texBase->set_texture(chunk->texture_set->filename(0));
            if (chunk->texture_set->num() > 1) _texRock->set_texture(chunk->texture_set->filename(1));
            if (chunk->texture_set->num() > 2) _texRiver->set_texture(chunk->texture_set->filename(2));
            if (chunk->texture_set->num() > 3) _texExtra->set_texture(chunk->texture_set->filename(3));
        }
        
        QMessageBox::information(this, "Palette Captured", "Successfully loaded the textures from the selected map chunk!");
    }

    void ProceduralToolWidget::onAddLayer() {
        addLayerUI(0, 0, 1337, 4, 2.0f, 0.5f, 0.005f, 1.0f);
    }

    void ProceduralToolWidget::onClearLayers() {
        for(auto& layer : _layers) {
            _layersLayout->removeWidget(layer.group);
            delete layer.group;
        }
        _layers.clear();
    }

    void ProceduralToolWidget::addLayerUI(int typeIdx, int blendIdx, int seed, int octaves, float scale, float persistence, float frequency, float weight) {
        LayerUI ui;
        ui.group = new QGroupBox(QString("Layer %1").arg(_layers.size() + 1));
        auto form = new QFormLayout(ui.group);

        ui.type = new QComboBox(ui.group);
        ui.type->addItems({"Perlin", "Simplex", "Cellular", "CellularValue", "Value", "Ridged", "PingPong"});
        ui.type->setCurrentIndex(typeIdx);
        form->addRow("Type:", ui.type);

        ui.blend = new QComboBox(ui.group);
        ui.blend->addItems({"Add", "Subtract", "Min", "Max", "Multiply"});
        ui.blend->setCurrentIndex(blendIdx);
        form->addRow("Blend:", ui.blend);

        ui.seed = new QSpinBox(ui.group); ui.seed->setRange(0, 999999); ui.seed->setValue(seed);
        form->addRow("Seed:", ui.seed);

        ui.octaves = new QSpinBox(ui.group); ui.octaves->setRange(1, 10); ui.octaves->setValue(octaves);
        form->addRow("Octaves:", ui.octaves);

        ui.scale = new QDoubleSpinBox(ui.group); ui.scale->setRange(0.01, 100.0); ui.scale->setValue(scale); ui.scale->setSingleStep(0.1);
        form->addRow("Scale:", ui.scale);

        ui.persistence = new QDoubleSpinBox(ui.group); ui.persistence->setRange(0.01, 1.0); ui.persistence->setValue(persistence); ui.persistence->setSingleStep(0.05);
        form->addRow("Persistence:", ui.persistence);

        ui.frequency = new QDoubleSpinBox(ui.group); ui.frequency->setRange(0.001, 100.0); ui.frequency->setValue(frequency); ui.frequency->setSingleStep(0.01); ui.frequency->setDecimals(3);
        form->addRow("Frequency:", ui.frequency);

        ui.weight = new QDoubleSpinBox(ui.group); ui.weight->setRange(0.01, 10.0); ui.weight->setValue(weight); ui.weight->setSingleStep(0.1);
        form->addRow("Weight:", ui.weight);

        ui.removeBtn = new QPushButton("Remove Layer", ui.group);
        form->addRow(ui.removeBtn);

        connect(ui.removeBtn, &QPushButton::clicked, [this, group = ui.group]() {
            _layersLayout->removeWidget(group);
            delete group;
            _layers.erase(std::remove_if(_layers.begin(), _layers.end(), [&](const LayerUI& l){ return l.group == group; }), _layers.end());
            for (size_t i = 0; i < _layers.size(); ++i) {
                _layers[i].group->setTitle(QString("Layer %1").arg(i + 1));
            }
        });

        _layersLayout->addWidget(ui.group);
        _layers.push_back(ui);
    }

    void ProceduralToolWidget::onRandomizeSeeds() {
        for (auto& layer : _layers) {
            layer.seed->setValue(rand() % 100000);
        }
    }

    void ProceduralToolWidget::onLoadPreset() {
        int idx = _presetCombo->currentIndex();
        onClearLayers();
        _terraceSteps->setValue(0);
        _smoothingRadius->setValue(0);
        _islandMix->setValue(0.0);
        _exponent->setValue(1.0);
        _erosionIterations->setValue(100000);

        if (idx == 0) { // Default
            addLayerUI(0, 0, 42, 6, 2.0f, 0.5f, 0.005f, 1.0f);
            addLayerUI(4, 0, 142, 4, 2.0f, 0.5f, 0.01f, 0.3f);
        } else if (idx == 1) { // Mountain Range
            addLayerUI(4, 0, 1234, 8, 2.0f, 0.5f, 0.0025f, 1.5f);
            addLayerUI(0, 0, 1284, 4, 2.0f, 0.5f, 0.01f, 0.5f);
            _exponent->setValue(1.1); // Lowered from 1.5 to avoid spikes
            _erosionIterations->setValue(300000);
        } else if (idx == 2) { // WoWNoggit
            addLayerUI(0, 0, 9999, 6, 2.0f, 0.5f, 0.005f, 1.0f);
            addLayerUI(4, 0, 10099, 4, 2.0f, 0.5f, 0.0075f, 0.3f);
            addLayerUI(3, 2, 10199, 3, 2.0f, 0.5f, 0.01f, 0.4f); // Min
            _exponent->setValue(2.0);
            _smoothingRadius->setValue(2);
            _erosionIterations->setValue(200000);
        } else if (idx == 3) { // Islas
            addLayerUI(1, 0, 1111, 5, 2.0f, 0.5f, 0.015f, 1.2f);
            _islandMix->setValue(1.0);
            _exponent->setValue(1.2);
        } else if (idx == 4) { // Crateres
            addLayerUI(0, 0, 2222, 4, 2.0f, 0.5f, 0.005f, 0.3f);
            addLayerUI(2, 1, 2322, 2, 2.0f, 0.5f, 0.02f, 1.0f); // Subtract
            addLayerUI(4, 0, 2422, 3, 2.0f, 0.5f, 0.02f, 0.5f);
            _exponent->setValue(1.5);
        } else if (idx == 5) { // Picos y Valles
            addLayerUI(1, 0, 8888, 6, 2.0f, 0.5f, 0.004f, 0.8f); // Base Simplex
            addLayerUI(5, 0, 8889, 5, 2.0f, 0.5f, 0.008f, 1.5f); // Ridged peaks
            _exponent->setValue(2.5); // Aplanar valles y exagerar picos
            _erosionIterations->setValue(250000);
        } else if (idx == 6) { // Costa Montanosa
            addLayerUI(1, 0, 7777, 4, 2.0f, 0.5f, 0.003f, 1.0f); // Base suave y amplia
            addLayerUI(5, 0, 7778, 6, 2.0f, 0.5f, 0.007f, 2.0f); // Picos escarpados muy altos
            _exponent->setValue(1.8);
            _islandMix->setValue(0.75); // Hundir los bordes para crear costa/playas
            _terraceSteps->setValue(15); // Añadir terrazas para acantilados y zonas planas de playa
            _erosionIterations->setValue(300000);
        }
    }

    void ProceduralToolWidget::onLoadJson() {
        QString fileName = QFileDialog::getOpenFileName(this, "Load TerrainGen JSON", "", "JSON Files (*.json)");
        if (fileName.isEmpty()) return;

        try {
            std::ifstream file(fileName.toStdString());
            if (!file.is_open()) {
                QMessageBox::warning(this, "Error", "Could not open file.");
                return;
            }

            nlohmann::json j;
            file >> j;

            onClearLayers();

            if (j.contains("modifiers")) {
                auto& mods = j["modifiers"];
                if (mods.contains("terraceSteps")) _terraceSteps->setValue(mods["terraceSteps"].get<int>());
                if (mods.contains("smoothing_radius")) _smoothingRadius->setValue(mods["smoothing_radius"].get<int>());
                if (mods.contains("islandMix")) _islandMix->setValue(mods["islandMix"].get<float>());
                if (mods.contains("exponent")) _exponent->setValue(mods["exponent"].get<float>());
            }

            if (j.contains("num_droplets")) {
                _erosionIterations->setValue(j["num_droplets"].get<int>());
            }

            int baseSeed = j.value("seed", 0);

            if (j.contains("noiseLayers") && j["noiseLayers"].is_array()) {
                for (const auto& layer : j["noiseLayers"]) {
                    int typeIdx = layer.value("type", 0);
                    int blendIdx = layer.value("blend", 0);
                    int seed = baseSeed + layer.value("seedOffset", 0);
                    int octaves = layer.value("octaves", 4);
                    float scale = layer.value("scale", 2.0f);
                    float persistence = layer.value("persistence", 0.5f);
                    float frequency = layer.value("baseFrequency", 0.01f);
                    float weight = layer.value("weight", 1.0f);

                    addLayerUI(typeIdx, blendIdx, seed, octaves, scale, persistence, frequency, weight);
                }
            }

            QMessageBox::information(this, "Success", "Project loaded successfully from JSON.");
        } catch (const std::exception& e) {
            QMessageBox::critical(this, "Error", QString("Failed to parse JSON: %1").arg(e.what()));
        }
    }

    void ProceduralToolWidget::onApply()
    {
        if (!_mapView) {
            QMessageBox::warning(this, "Error", "No map view active.");
            return;
        }
        
        auto world = _mapView->getWorld();
        if (!world) {
            QMessageBox::warning(this, "Error", "No world loaded.");
            return;
        }

        // 1. Detección de todos los MapTiles cargados
        int minTileX = 64, maxTileX = -1, minTileY = 64, maxTileY = -1;
        std::vector<MapTile*> loadedTiles;
        
        for (int ty = 0; ty < 64; ++ty) {
            for (int tx = 0; tx < 64; ++tx) {
                // Filter by target area if enabled
                if (_useTargetArea->isChecked()) {
                    if (!_proceduralSelection[tx * 64 + ty]) {
                        continue;
                    }
                }
                MapTile* tile = world->mapIndex.getTile(TileIndex(tx, ty));
                if (tile) {
                    minTileX = std::min(minTileX, tx);
                    maxTileX = std::max(maxTileX, tx);
                    minTileY = std::min(minTileY, ty);
                    maxTileY = std::max(maxTileY, ty);
                    loadedTiles.push_back(tile);
                }
            }
        }
        
        if (loadedTiles.empty()) {
            QMessageBox::warning(this, "Error", "No MapTiles are currently loaded.");
            return;
        }

        int tilesWidth = (maxTileX - minTileX + 1);
        int tilesHeight = (maxTileY - minTileY + 1);
        
        // Cada ADT tiene 16 chunks de 8 quads. Para poder muestrear los vértices internos (que están en el centro del quad),
        // doblamos la resolución. Así cada quad mide 2x2.
        int globalWidth = tilesWidth * 256 + 1;
        int globalHeight = tilesHeight * 256 + 1;

        // Build Noise Layers
        std::vector<NoiseLayer> layers;
        for (auto& uiLayer : _layers) {
            NoiseLayer layer;
            layer.type = static_cast<NoiseType>(uiLayer.type->currentIndex());
            layer.blend = static_cast<BlendMode>(uiLayer.blend->currentIndex());
            layer.seedOffset = uiLayer.seed->value();
            layer.octaves = uiLayer.octaves->value();
            layer.scale = uiLayer.scale->value();
            layer.persistence = uiLayer.persistence->value();
            layer.baseFrequency = uiLayer.frequency->value();
            layer.weight = uiLayer.weight->value();
            layer.enabled = true;
            layers.push_back(layer);
        }

        // Modifiers
        ModifierParams modParams;
        modParams.terraceSteps = _terraceSteps->value();
        modParams.smoothing_radius = _smoothingRadius->value();
        modParams.islandMix = _islandMix->value();
        modParams.exponent = _exponent->value();

        // 2. Generate Global Heightmap
        LogDebug << "Generating global heightmap..." << std::endl;
        int offsetX = minTileX * 256;
        int offsetY = minTileY * 256;
        Heightmap hmap = HeightmapGenerator::generate(0, offsetX, offsetY, globalWidth, globalHeight, layers, modParams);

        // 3. Erosion
        LogDebug << "Running erosion..." << std::endl;
        Heightmap flow(globalWidth, globalHeight); // To store water accumulation for splatmaps
        if (_erosionIterations->value() > 0) {
            ErosionParams eParams;
            eParams.num_droplets = _erosionIterations->value();
            HydraulicErosion::erode(hmap, eParams);
            flow = Hydrology::calculateFlowAccumulation(hmap);
        }

        if (modParams.smoothing_radius > 0) {
            LogDebug << "Running smoothing..." << std::endl;
            Filters::fastBoxBlur(hmap, modParams.smoothing_radius);
        }

        LogDebug << "Applying terrain to chunks..." << std::endl;

        auto bilerp = [](const Heightmap& map, float px, float py, int w, int h) {
            int ix = std::max(0, std::min(w - 2, (int)px));
            int iy = std::max(0, std::min(h - 2, (int)py));
            float fx = px - ix;
            float fy = py - iy;
            float h00 = map.at(ix, iy);
            float h10 = map.at(ix+1, iy);
            float h01 = map.at(ix, iy+1);
            float h11 = map.at(ix+1, iy+1);
            float h0 = h00 + fx * (h10 - h00);
            float h1 = h01 + fx * (h11 - h01);
            return h0 + fy * (h1 - h0);
        };

        // 4. Write back to all chunks in all loaded tiles
        for (MapTile* tile : loadedTiles) {
            int tx = (int)tile->index.x;
            int ty = (int)tile->index.z;
            
            LogDebug << "Processing tile " << tx << ", " << ty << std::endl;
            for (int cy = 0; cy < 16; ++cy) {
                for (int cx = 0; cx < 16; ++cx) {
                    MapChunk* chunk = tile->getChunk(cx, cy);
                    if (!chunk) continue;

                    // Outer vertices: 9x9
                    // Global index in the big tile grid
                    for (int y = 0; y < 9; ++y) {
                        for (int x = 0; x < 9; ++x) {
                            int vIdx = y * 17 + x; // Interleaved outer index
                            int gx = (tx - minTileX) * 256 + cx * 16 + x * 2;
                            int gy = (ty - minTileY) * 256 + cy * 16 + y * 2;
                            
                            // Transform 0-1 height to WoW scale
                            float heightWow = hmap.at(gx, gy) * 200.0f;
                            chunk->mVertices[vIdx].y = heightWow;
                        }
                    }

                    // Inner vertices: 8x8 (sampled from noise instead of average!)
                    for (int y = 0; y < 8; ++y) {
                        for (int x = 0; x < 8; ++x) {
                            int vIdx = y * 17 + x + 9; // Interleaved inner index
                            int gx = (tx - minTileX) * 256 + cx * 16 + x * 2 + 1;
                            int gy = (ty - minTileY) * 256 + cy * 16 + y * 2 + 1;
                            
                            float heightWow = hmap.at(gx, gy) * 200.0f;
                            chunk->mVertices[vIdx].y = heightWow;
                        }
                    }

                    // 5. Splatmaps (Texturas)
                    TextureSet* tset = chunk->getTextureSet();
                    if (tset) {
                        LogDebug << "Erasing textures for chunk " << cx << ", " << cy << std::endl;
                        chunk->eraseTextures();
                        LogDebug << "Adding texture Base: " << _texBase->filename() << std::endl;
                        chunk->addTexture(scoped_blp_texture_reference(_texBase->filename(), Noggit::NoggitRenderContext::MAP_VIEW));
                        LogDebug << "Adding texture Rock: " << _texRock->filename() << std::endl;
                        chunk->addTexture(scoped_blp_texture_reference(_texRock->filename(), Noggit::NoggitRenderContext::MAP_VIEW));
                        LogDebug << "Adding texture River: " << _texRiver->filename() << std::endl;
                        chunk->addTexture(scoped_blp_texture_reference(_texRiver->filename(), Noggit::NoggitRenderContext::MAP_VIEW));
                        LogDebug << "Adding texture Extra: " << _texExtra->filename() << std::endl;
                        chunk->addTexture(scoped_blp_texture_reference(_texExtra->filename(), Noggit::NoggitRenderContext::MAP_VIEW));

                        LogDebug << "Getting alphamaps..." << std::endl;
                        auto alphas = tset->getAlphamaps();
                        if (alphas && (*alphas)[0] && (*alphas)[1] && (*alphas)[2]) {
                            Alphamap* rockLayer = (*alphas)[0].get();
                            Alphamap* riverLayer = (*alphas)[1].get();
                            Alphamap* extraLayer = (*alphas)[2].get();

                            LogDebug << "Setting alpha values..." << std::endl;
                            for (int ay = 0; ay < 64; ++ay) {
                                for (int ax = 0; ax < 64; ++ax) {
                                    float px = (tx - minTileX) * 256 + cx * 16 + (ax / 64.0f) * 16.0f;
                                    float py = (ty - minTileY) * 256 + cy * 16 + (ay / 64.0f) * 16.0f;
                                    
                                    // Smooth slope calculation using physical WoW units
                                    float height = bilerp(hmap, px, py, globalWidth, globalHeight);
                                    float hLeft = bilerp(hmap, px - 0.5f, py, globalWidth, globalHeight);
                                    float hRight = bilerp(hmap, px + 0.5f, py, globalWidth, globalHeight);
                                    float hUp = bilerp(hmap, px, py - 0.5f, globalWidth, globalHeight);
                                    float hDown = bilerp(hmap, px, py + 0.5f, globalWidth, globalHeight);

                                    // La altura en WoW va multiplicada por 200.0f.
                                    // La distancia horizontal entre dos píxeles del heightmap es 533.33 / 256 = 2.0833f yardas.
                                    // factor = 200.0f / 2.0833f = ~96.0f
                                    float slopeX = (hRight - hLeft) * 96.0f;
                                    float slopeY = (hDown - hUp) * 96.0f;
                                    float slope = std::sqrt(slopeX*slopeX + slopeY*slopeY);
                                    
                                    // Flujo de agua
                                    float water = bilerp(flow, px, py, globalWidth, globalHeight);

                                    // Smooth blending
                                    // slope = 1.0 significa 45 grados. Empezamos a mezclar roca a partir de pendiente 0.6 (~31 grados) hasta 1.0 (45 grados)
                                    float rockIntensity = std::max(0.0f, std::min(1.0f, (slope - 0.6f) / 0.4f));
                                    float waterIntensity = std::max(0.0f, std::min(1.0f, (water - 3.0f) / 7.0f));
                                    float extraIntensity = std::max(0.0f, std::min(1.0f, (height - 0.55f) / 0.15f)); // Snow starts at 0.55

                                    // Water dominates rock, rock dominates extra, water dominates extra
                                    rockIntensity = std::max(0.0f, rockIntensity - waterIntensity);
                                    extraIntensity = std::max(0.0f, extraIntensity - rockIntensity - waterIntensity);

                                    uint8_t rockAlpha = static_cast<uint8_t>(rockIntensity * 255.0f);
                                    uint8_t waterAlpha = static_cast<uint8_t>(waterIntensity * 255.0f);
                                    uint8_t extraAlpha = static_cast<uint8_t>(extraIntensity * 255.0f);

                                    rockLayer->setAlpha(ay * 64 + ax, rockAlpha);
                                    riverLayer->setAlpha(ay * 64 + ax, waterAlpha);
                                    extraLayer->setAlpha(ay * 64 + ax, extraAlpha);
                                }
                            }
                            LogDebug << "Registering alphamap update..." << std::endl;
                            chunk->registerChunkUpdate(ALPHAMAP);
                        } else {
                            LogDebug << "Failed to retrieve alphamaps!" << std::endl;
                        }
                    } else {
                        LogDebug << "No texture set for chunk " << cx << ", " << cy << std::endl;
                    }

                    LogDebug << "Updating vertices..." << std::endl;
                    chunk->recalcNorms();
                    chunk->registerChunkUpdate(VERTEX);
                    chunk->registerChunkUpdate(NORMALS);
                }
            }
        }
        
        QMessageBox::information(this, "Success", QString("Procedural Generation applied to %1 MapTiles!").arg(loadedTiles.size()));
    }

    void ProceduralToolWidget::onGenerateLightmap()
    {
        if (!_mapView) return;
        auto world = _mapView->getWorld();
        if (!world) return;

        float azimuth = _lmAzimuth->value() * (3.1415926535f / 180.0f);
        float elevation = _lmElevation->value() * (3.1415926535f / 180.0f);
        float ambient = _lmAmbient->value();
        float intensity = _lmIntensity->value();
        float zScale = _lmZScale->value();
        int step = _lmStep->value();

        glm::vec3 lightDir(
            std::cos(elevation) * std::cos(azimuth),
            std::sin(elevation),
            std::cos(elevation) * std::sin(azimuth)
        );
        lightDir = glm::normalize(lightDir);

        for (int ty = 0; ty < 64; ++ty) {
            for (int tx = 0; tx < 64; ++tx) {
                if (_useTargetArea->isChecked() && !_proceduralSelection[tx * 64 + ty]) continue;
                
                MapTile* tile = world->mapIndex.getTile(TileIndex(tx, ty));
                if (!tile) continue;

                for (int cy = 0; cy < 16; ++cy) {
                    for (int cx = 0; cx < 16; ++cx) {
                        MapChunk* chunk = tile->getChunk(cx, cy);
                        if (!chunk) continue;

                        chunk->initMCCV(); // Ensures memory for MCCV exists

                        for (int i = 0; i < mapbufsize; ++i) {
                            glm::vec3 pos = chunk->mVertices[i];
                            
                            // Sample heights using global GetVertex
                            glm::vec3 vL, vR, vD, vU;
                            float hL = world->GetVertex(pos.x - step, pos.z, &vL) ? vL.y : pos.y;
                            float hR = world->GetVertex(pos.x + step, pos.z, &vR) ? vR.y : pos.y;
                            float hD = world->GetVertex(pos.x, pos.z - step, &vD) ? vD.y : pos.y;
                            float hU = world->GetVertex(pos.x, pos.z + step, &vU) ? vU.y : pos.y;

                            glm::vec3 normal(
                                (hL - hR) * zScale / (2.0f * step),
                                1.0f,
                                (hD - hU) * zScale / (2.0f * step)
                            );
                            normal = glm::normalize(normal);

                            float lambert = std::max(0.0f, glm::dot(normal, lightDir));
                            float finalLight = std::clamp(ambient + lambert * intensity, 0.0f, 2.0f);
                            
                            chunk->mccv[i] = glm::vec3(finalLight, finalLight, finalLight);
                        }
                        chunk->registerChunkUpdate(MCCV);
                    }
                }
            }
        }
    }

    void ProceduralToolWidget::onTestLightmap()
    {
        if (!_mapView) return;
        auto world = _mapView->getWorld();
        if (!world) return;

        float azimuth = _lmAzimuth->value() * (3.1415926535f / 180.0f);
        float elevation = _lmElevation->value() * (3.1415926535f / 180.0f);
        float ambient = _lmAmbient->value();
        float intensity = _lmIntensity->value();
        float zScale = _lmZScale->value();
        int step = _lmStep->value();

        glm::vec3 lightDir(
            std::cos(elevation) * std::cos(azimuth),
            std::sin(elevation),
            std::cos(elevation) * std::sin(azimuth)
        );
        lightDir = glm::normalize(lightDir);

        for (int ty = 0; ty < 64; ++ty) {
            for (int tx = 0; tx < 64; ++tx) {
                if (_useTargetArea->isChecked() && !_proceduralSelection[tx * 64 + ty]) continue;
                
                MapTile* tile = world->mapIndex.getTile(TileIndex(tx, ty));
                if (!tile) continue;

                for (int cy = 0; cy < 16; ++cy) {
                    for (int cx = 0; cx < 16; ++cx) {
                        MapChunk* chunk = tile->getChunk(cx, cy);
                        if (!chunk) continue;

                        chunk->initMCCV();

                        for (int i = 0; i < mapbufsize; ++i) {
                            glm::vec3 pos = chunk->mVertices[i];
                            
                            glm::vec3 vL, vR, vD, vU;
                            float hL = world->GetVertex(pos.x - step, pos.z, &vL) ? vL.y : pos.y;
                            float hR = world->GetVertex(pos.x + step, pos.z, &vR) ? vR.y : pos.y;
                            float hD = world->GetVertex(pos.x, pos.z - step, &vD) ? vD.y : pos.y;
                            float hU = world->GetVertex(pos.x, pos.z + step, &vU) ? vU.y : pos.y;

                            glm::vec3 normal(
                                (hL - hR) * zScale / (2.0f * step),
                                1.0f,
                                (hD - hU) * zScale / (2.0f * step)
                            );
                            normal = glm::normalize(normal);

                            float lambert = std::max(0.0f, glm::dot(normal, lightDir));
                            float finalLight = std::clamp(ambient + lambert * intensity, 0.0f, 2.0f);
                            
                            // Test mode Heatmap (Blue=Shadow, Green=Neutral, Red=Highlight)
                            float r = 0.0f, g = 0.0f, b = 0.0f;
                            if (finalLight < 1.0f) {
                                float t = finalLight; // 0.0 to 1.0
                                b = (1.0f - t) * 2.0f;
                                g = t * 2.0f;
                            } else {
                                float t = finalLight - 1.0f; // 0.0 to 1.0
                                g = (1.0f - t) * 2.0f;
                                r = t * 2.0f;
                            }
                            
                            chunk->mccv[i] = glm::vec3(r, g, b);
                        }
                        chunk->registerChunkUpdate(MCCV);
                    }
                }
            }
        }
    }

    void ProceduralToolWidget::onClearLightmap()
    {
        if (!_mapView) return;
        auto world = _mapView->getWorld();
        if (!world) return;

        for (int ty = 0; ty < 64; ++ty) {
            for (int tx = 0; tx < 64; ++tx) {
                if (_useTargetArea->isChecked() && !_proceduralSelection[tx * 64 + ty]) continue;
                
                MapTile* tile = world->mapIndex.getTile(TileIndex(tx, ty));
                if (!tile) continue;

                for (int cy = 0; cy < 16; ++cy) {
                    for (int cx = 0; cx < 16; ++cx) {
                        MapChunk* chunk = tile->getChunk(cx, cy);
                        if (!chunk) continue;

                        chunk->initMCCV();

                        for (int i = 0; i < mapbufsize; ++i) {
                            chunk->mccv[i] = glm::vec3(1.0f, 1.0f, 1.0f); // Neutro default
                        }
                        chunk->registerChunkUpdate(MCCV);
                    }
                }
            }
        }
    }

    void ProceduralToolWidget::reloadLmPresets() {
        _lmPresetCombo->blockSignals(true);
        _lmPresetCombo->clear();
        _lmPresetCombo->addItem("Custom");
        
        QSettings settings("Noggit", "LightmapPresets");
        if (!settings.childGroups().contains("Luces ElSuscriptor")) {
            settings.beginGroup("Luces ElSuscriptor");
            settings.setValue("az", 63.43);
            settings.setValue("el", 87.17);
            settings.setValue("am", 0.70);
            settings.setValue("in", 1.00);
            settings.setValue("zs", 3.00);
            settings.setValue("st", 20);
            settings.endGroup();
        }
        
        for (const QString& group : settings.childGroups()) {
            _lmPresetCombo->addItem(group);
        }
        _lmPresetCombo->blockSignals(false);
    }

    void ProceduralToolWidget::onSaveLmPreset() {
        bool ok;
        QString text = QInputDialog::getText(this, "Save Lightmap Preset",
                                             "Preset Name:", QLineEdit::Normal,
                                             "", &ok);
        if (ok && !text.isEmpty()) {
            QSettings settings("Noggit", "LightmapPresets");
            settings.beginGroup(text);
            settings.setValue("az", _lmAzimuth->value());
            settings.setValue("el", _lmElevation->value());
            settings.setValue("am", _lmAmbient->value());
            settings.setValue("in", _lmIntensity->value());
            settings.setValue("zs", _lmZScale->value());
            settings.setValue("st", _lmStep->value());
            settings.endGroup();
            reloadLmPresets();
            _lmPresetCombo->setCurrentText(text);
        }
    }

    void ProceduralToolWidget::onDeleteLmPreset() {
        QString current = _lmPresetCombo->currentText();
        if (current.isEmpty() || current == "Custom" || current == "Luces ElSuscriptor") return;
        
        QSettings settings("Noggit", "LightmapPresets");
        settings.remove(current);
        reloadLmPresets();
    }

    void ProceduralToolWidget::onPresetChanged(int index)
    {
        QString current = _lmPresetCombo->currentText();
        if (current == "Custom" || current.isEmpty()) return;
        
        QSettings settings("Noggit", "LightmapPresets");
        settings.beginGroup(current);
        _lmAzimuth->setValue(settings.value("az").toDouble());
        _lmElevation->setValue(settings.value("el").toDouble());
        _lmAmbient->setValue(settings.value("am").toDouble());
        _lmIntensity->setValue(settings.value("in").toDouble());
        _lmZScale->setValue(settings.value("zs").toDouble());
        _lmStep->setValue(settings.value("st").toInt());
        settings.endGroup();
    }
}
