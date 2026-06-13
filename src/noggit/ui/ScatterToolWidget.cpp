// This file is part of Noggit3, licensed under GNU General Public License (version 3).

#include "ScatterToolWidget.hpp"
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <noggit/MapView.h>
#include <noggit/World.h>
#include <noggit/SceneObject.hpp>
#include <noggit/ModelInstance.h>
#include <noggit/WMOInstance.h>
#include <noggit/Selection.h>
#include <QShortcut>

namespace Noggit::Ui {
    ScatterToolWidget::ScatterToolWidget(MapView* mapView, QWidget* parent)
        : QWidget(parent), _mapView(mapView)
    {
        auto layout = new QVBoxLayout(this);
        
        // --- Palette & Presets Group ---
        auto presetGroup = new QGroupBox("Palette & Presets");
        auto presetLayout = new QVBoxLayout(presetGroup);
        
        _btnImportFromMap = new QPushButton("Import from Map", this);
        _btnImportFromMap->setToolTip("Importa todos los modelos (M2 y WMO) que existen en el mapa actual a la lista de pintado.");
        presetLayout->addWidget(_btnImportFromMap);

        auto presetButtonsLayout = new QHBoxLayout();
        _btnSavePreset = new QPushButton("Save Preset", this);
        _btnSavePreset->setToolTip("Guarda la configuración actual y la lista de modelos en un archivo.");
        _btnLoadPreset = new QPushButton("Load Preset", this);
        _btnLoadPreset->setToolTip("Carga una configuración y lista de modelos desde un archivo.");
        presetButtonsLayout->addWidget(_btnSavePreset);
        presetButtonsLayout->addWidget(_btnLoadPreset);
        presetLayout->addLayout(presetButtonsLayout);
        
        layout->addWidget(presetGroup);
        
        // --- Scatter Settings Group ---
        auto group = new QGroupBox("Scatter Settings");
        auto form = new QFormLayout(group);
        
        _modelLabel = new QLabel("None", this);
        _modelLabel->setToolTip("Muestra la ruta del modelo actual seleccionado en el Asset Browser.");
        
        auto addRemoveLayout = new QHBoxLayout();
        _btnAddModel = new QPushButton("+", this);
        _btnAddModel->setToolTip("Añade el modelo seleccionado en el Asset Browser a la lista de pintado.");
        _btnRemoveModel = new QPushButton("-", this);
        _btnRemoveModel->setToolTip("Elimina el modelo seleccionado de la lista.");
        
        _btnAddFromSelection = new QPushButton("Add Selection", this);
        _btnAddFromSelection->setToolTip("Añade todos los objetos actualmente seleccionados en el mapa a la lista.");
        
        addRemoveLayout->addWidget(_modelLabel);
        addRemoveLayout->addWidget(_btnAddModel);
        addRemoveLayout->addWidget(_btnRemoveModel);
        addRemoveLayout->addWidget(_btnAddFromSelection);
        
        form->addRow("Asset Browser:", addRemoveLayout);
        
        _modelListWidget = new QListWidget(this);
        _modelListWidget->setToolTip("Lista de modelos que se colocarán aleatoriamente con el pincel.");
        form->addRow("Scatter List:", _modelListWidget);

        connect(_btnAddFromSelection, &QPushButton::clicked, this, [this]() {
            if (!_mapView || !_mapView->getWorld()) return;
            auto const& sel = _mapView->getWorld()->current_selection();
            for (auto const& item : sel) {
                if (item.index() == eEntry_Object) {
                    auto obj = std::get<selected_object_type>(item);
                    if (obj && (obj->which() == eMODEL || obj->which() == eWMO)) {
                        auto asyncModel = obj->instance_model();
                        if (asyncModel) {
                            addModelToScatterList(QString::fromStdString(asyncModel->file_key().filepath()));
                        }
                    }
                }
            }
        });

        auto* shortcutCtrlC = new QShortcut(QKeySequence("Ctrl+C"), this);
        connect(shortcutCtrlC, &QShortcut::activated, _btnAddFromSelection, &QPushButton::click);

        connect(_btnAddModel, &QPushButton::clicked, this, [this]() {
            if (!_selectedAssetBrowserModel.isEmpty() && _selectedAssetBrowserModel != "None") {
                // Ensure it's not already in the list
                for (int i = 0; i < _modelListWidget->count(); ++i) {
                    if (_modelListWidget->item(i)->data(Qt::UserRole).toString() == _selectedAssetBrowserModel) {
                        return; // already exists
                    }
                }
                
                QString display = _selectedAssetBrowserModel;
                if (display.length() > 30) {
                    display = "..." + display.right(27);
                }
                auto* item = new QListWidgetItem(display);
                item->setData(Qt::UserRole, _selectedAssetBrowserModel);
                _modelListWidget->addItem(item);
                _modelsList.push_back(_selectedAssetBrowserModel);
            }
        });

        connect(_btnRemoveModel, &QPushButton::clicked, this, [this]() {
            auto items = _modelListWidget->selectedItems();
            if (!items.isEmpty()) {
                QString toRemove = items.first()->data(Qt::UserRole).toString();
                auto it = std::find(_modelsList.begin(), _modelsList.end(), toRemove);
                if (it != _modelsList.end()) {
                    _modelsList.erase(it);
                }
                delete _modelListWidget->takeItem(_modelListWidget->row(items.first()));
            }
        });

        _brushRadius = new QDoubleSpinBox(this);
        _brushRadius->setRange(1.0, 100.0);
        _brushRadius->setValue(20.0);
        _brushRadius->setToolTip("Define el tamaño del área circular donde aparecerán los modelos.");
        form->addRow("Brush Radius:", _brushRadius);

        _density = new QSpinBox(this);
        _density->setRange(1, 100);
        _density->setValue(10);
        _density->setToolTip("Controla la cantidad de objetos que se generan mientras arrastras el ratón.");
        form->addRow("Density (per click/tick):", _density);

        _minHeight = new QDoubleSpinBox(this);
        _minHeight->setRange(-10000.0, 10000.0);
        _minHeight->setValue(-500.0);
        _minHeight->setToolTip("Altura mínima del terreno permitida. Si el suelo está por debajo de esto, no se pintará.");
        form->addRow("Min Height Constraint:", _minHeight);

        _maxHeight = new QDoubleSpinBox(this);
        _maxHeight->setRange(-10000.0, 10000.0);
        _maxHeight->setValue(500.0);
        _maxHeight->setToolTip("Altura máxima del terreno permitida. Útil para evitar pintar árboles en cimas altas.");
        form->addRow("Max Height Constraint:", _maxHeight);

        _minScale = new QDoubleSpinBox(this);
        _minScale->setRange(0.1, 10.0);
        _minScale->setValue(0.8);
        _minScale->setSingleStep(0.1);
        _minScale->setToolTip("Escala mínima aleatoria para el modelo.");
        form->addRow("Min Scale:", _minScale);

        _maxScale = new QDoubleSpinBox(this);
        _maxScale->setRange(0.1, 10.0);
        _maxScale->setValue(1.2);
        _maxScale->setSingleStep(0.1);
        _maxScale->setToolTip("Escala máxima aleatoria para el modelo.");
        form->addRow("Max Scale:", _maxScale);

        _minDistance = new QDoubleSpinBox(this);
        _minDistance->setRange(0.0, 100.0);
        _minDistance->setValue(5.0);
        _minDistance->setToolTip("Distancia mínima que debe haber entre los modelos para que no se superpongan o se pisen entre ellos.");
        form->addRow("Min Distance:", _minDistance);

        _randomRotation = new QCheckBox("Randomize Z Rotation", this);
        _randomRotation->setChecked(true);
        _randomRotation->setToolTip("Gira cada objeto aleatoriamente para que el bosque no parezca repetitivo.");
        form->addRow(_randomRotation);

        layout->addWidget(group);
        layout->addStretch();

        // --- Connections for Preset Buttons ---
        connect(_btnImportFromMap, &QPushButton::clicked, this, [this]() {
            if (!_mapView || !_mapView->getWorld()) return;
            auto world = _mapView->getWorld();

            world->getModelInstanceStorage().for_each_m2_instance([this](ModelInstance& instance) {
                addModelToScatterList(QString::fromStdString(instance.model->file_key().filepath()));
            });
            world->getModelInstanceStorage().for_each_wmo_instance([this](WMOInstance& instance) {
                addModelToScatterList(QString::fromStdString(instance.wmo->file_key().filepath()));
            });
        });

        connect(_btnSavePreset, &QPushButton::clicked, this, [this]() {
            QString fileName = QFileDialog::getSaveFileName(this, "Save Scatter Preset", "", "JSON Files (*.json)");
            if (fileName.isEmpty()) return;

            QJsonObject root;
            root["brushRadius"] = _brushRadius->value();
            root["density"] = _density->value();
            root["minHeight"] = _minHeight->value();
            root["maxHeight"] = _maxHeight->value();
            root["minScale"] = _minScale->value();
            root["maxScale"] = _maxScale->value();
            root["minDistance"] = _minDistance->value();
            root["randomRotation"] = _randomRotation->isChecked();

            QJsonArray modelsArray;
            for (const auto& model : _modelsList) {
                modelsArray.append(model);
            }
            root["models"] = modelsArray;

            QJsonDocument doc(root);
            QFile file(fileName);
            if (file.open(QIODevice::WriteOnly)) {
                file.write(doc.toJson());
                file.close();
            }
        });

        connect(_btnLoadPreset, &QPushButton::clicked, this, [this]() {
            QString fileName = QFileDialog::getOpenFileName(this, "Load Scatter Preset", "", "JSON Files (*.json)");
            if (fileName.isEmpty()) return;

            QFile file(fileName);
            if (!file.open(QIODevice::ReadOnly)) return;

            QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
            file.close();

            if (!doc.isObject()) return;
            QJsonObject root = doc.object();

            if (root.contains("brushRadius")) _brushRadius->setValue(root["brushRadius"].toDouble());
            if (root.contains("density")) _density->setValue(root["density"].toInt());
            if (root.contains("minHeight")) _minHeight->setValue(root["minHeight"].toDouble());
            if (root.contains("maxHeight")) _maxHeight->setValue(root["maxHeight"].toDouble());
            if (root.contains("minScale")) _minScale->setValue(root["minScale"].toDouble());
            if (root.contains("maxScale")) _maxScale->setValue(root["maxScale"].toDouble());
            if (root.contains("minDistance")) _minDistance->setValue(root["minDistance"].toDouble());
            if (root.contains("randomRotation")) _randomRotation->setChecked(root["randomRotation"].toBool());

            if (root.contains("models") && root["models"].isArray()) {
                _modelListWidget->clear();
                _modelsList.clear();
                
                QJsonArray modelsArray = root["models"].toArray();
                for (const auto& val : modelsArray) {
                    addModelToScatterList(val.toString());
                }
            }
        });

        connect(_modelListWidget, &QListWidget::itemClicked, this, [this](QListWidgetItem* item) {
            if (item) {
                QString path = item->data(Qt::UserRole).toString();
                _selectedAssetBrowserModel = path;
                QString display = path;
                if (display.length() > 30) {
                    display = "..." + display.right(27);
                }
                _modelLabel->setText(display);
            }
        });
    }

    void ScatterToolWidget::setSelectedModel(const QString& modelPath)
    {
        _selectedAssetBrowserModel = modelPath;
        QString display = modelPath;
        if (display.length() > 30) {
            display = "..." + display.right(27);
        }
        _modelLabel->setText(display);
    }

    std::vector<QString> ScatterToolWidget::getSelectedModels() const
    {
        return _modelsList;
    }

    void ScatterToolWidget::addModelToScatterList(const QString& path)
    {
        if (path.isEmpty() || _modelsList.end() != std::find(_modelsList.begin(), _modelsList.end(), path))
            return;
            
        QString display = path;
        if (display.length() > 30) {
            display = "..." + display.right(27);
        }
        auto* item = new QListWidgetItem(display);
        item->setData(Qt::UserRole, path);
        _modelListWidget->addItem(item);
        _modelsList.push_back(path);
    }
}
