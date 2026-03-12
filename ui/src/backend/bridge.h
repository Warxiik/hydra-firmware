#pragma once

#include <QObject>
#include <QString>
#include <QtQml/qqmlregistration.h>

namespace hydra::ui {

class PrinterBridge : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString state READ state NOTIFY stateChanged)
    Q_PROPERTY(double progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(int currentLayer READ currentLayer NOTIFY progressChanged)
    Q_PROPERTY(int totalLayers READ totalLayers NOTIFY progressChanged)
    Q_PROPERTY(int printTimeElapsed READ printTimeElapsed NOTIFY progressChanged)
    Q_PROPERTY(int printTimeRemaining READ printTimeRemaining NOTIFY progressChanged)
    Q_PROPERTY(double nozzle0Temp READ nozzle0Temp NOTIFY tempsChanged)
    Q_PROPERTY(double nozzle0Target READ nozzle0Target NOTIFY tempsChanged)
    Q_PROPERTY(double nozzle1Temp READ nozzle1Temp NOTIFY tempsChanged)
    Q_PROPERTY(double nozzle1Target READ nozzle1Target NOTIFY tempsChanged)
    Q_PROPERTY(double bedTemp READ bedTemp NOTIFY tempsChanged)
    Q_PROPERTY(double bedTarget READ bedTarget NOTIFY tempsChanged)

public:
    explicit PrinterBridge(QObject *parent = nullptr);

    QString state() const { return state_; }
    double progress() const { return progress_; }
    int currentLayer() const { return current_layer_; }
    int totalLayers() const { return total_layers_; }
    int printTimeElapsed() const { return elapsed_s_; }
    int printTimeRemaining() const { return remaining_s_; }
    double nozzle0Temp() const { return n0_temp_; }
    double nozzle0Target() const { return n0_target_; }
    double nozzle1Temp() const { return n1_temp_; }
    double nozzle1Target() const { return n1_target_; }
    double bedTemp() const { return bed_temp_; }
    double bedTarget() const { return bed_target_; }

public slots:
    void startPrint(const QString& filePath);
    void pausePrint();
    void resumePrint();
    void cancelPrint();
    void homeAll();
    void setNozzleTemp(int nozzle, double temp);
    void setBedTemp(double temp);
    void jogAxis(const QString& axis, double distance);
    void setFanSpeed(int fan, int percent);

signals:
    void stateChanged();
    void tempsChanged();
    void progressChanged();
    void errorOccurred(const QString& message);
    void filamentChangeRequired(int nozzle);

private:
    QString state_ = "Idle";
    double progress_ = 0.0;
    int current_layer_ = 0, total_layers_ = 0;
    int elapsed_s_ = 0, remaining_s_ = 0;
    double n0_temp_ = 0, n0_target_ = 0;
    double n1_temp_ = 0, n1_target_ = 0;
    double bed_temp_ = 0, bed_target_ = 0;
};

} // namespace hydra::ui
