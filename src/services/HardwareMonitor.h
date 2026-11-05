#pragma once
#include <QObject>

struct udev;
struct udev_monitor;

class HardwareMonitor : public QObject {
    Q_OBJECT
public:
    explicit HardwareMonitor(QObject *parent = nullptr);
    ~HardwareMonitor() override;
signals:
    void changed();
private:
    udev *context_ = nullptr;
    udev_monitor *monitor_ = nullptr;
};
