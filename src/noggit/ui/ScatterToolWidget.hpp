// This file is part of Noggit3, licensed under GNU General Public License (version 3).

#pragma once

#include <QWidget>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QFormLayout>
#include <QString>
#include <QListWidget>
#include <QPushButton>

class MapView;

namespace Noggit::Ui {
    class ScatterToolWidget : public QWidget {
        Q_OBJECT
    public:
        explicit ScatterToolWidget(MapView* mapView, QWidget* parent = nullptr);
        ~ScatterToolWidget() override = default;

        void setSelectedModel(const QString& modelPath);
        std::vector<QString> getSelectedModels() const;
        void addModelToScatterList(const QString& path);

        float getBrushRadius() const { return _brushRadius->value(); }
        int getDensity() const { return _density->value(); }
        float getMinHeight() const { return _minHeight->value(); }
        float getMaxHeight() const { return _maxHeight->value(); }
        float getMinScale() const { return _minScale->value(); }
        float getMaxScale() const { return _maxScale->value(); }
        float getMinDistance() const { return _minDistance->value(); }
        bool getRandomRotation() const { return _randomRotation->isChecked(); }

    private:
        MapView* _mapView;
        QString _selectedAssetBrowserModel;
        std::vector<QString> _modelsList;

        QLabel* _modelLabel;
        QListWidget* _modelListWidget;
        QPushButton* _btnAddModel;
        QPushButton* _btnRemoveModel;
        
        QPushButton* _btnAddFromSelection;
        QPushButton* _btnImportFromMap;
        QPushButton* _btnSavePreset;
        QPushButton* _btnLoadPreset;

        QDoubleSpinBox* _brushRadius;
        QSpinBox* _density;
        QDoubleSpinBox* _minHeight;
        QDoubleSpinBox* _maxHeight;
        QDoubleSpinBox* _minScale;
        QDoubleSpinBox* _maxScale;
        QDoubleSpinBox* _minDistance;
        QCheckBox* _randomRotation;
    };
}
