#include "HardwareMonitor.h"

#include <QSocketNotifier>
#include <libudev.h>

HardwareMonitor::HardwareMonitor(QObject *parent) : QObject(parent)
{
    context_ = udev_new();
    if (!context_) return;
    monitor_ = udev_monitor_new_from_netlink(context_, "udev");
    if (!monitor_) return;
    for (const char *subsystem : {"pci", "usb", "sound", "bluetooth", "video4linux", "input", "net", "thunderbolt", "acpi"})
        udev_monitor_filter_add_match_subsystem_devtype(monitor_, subsystem, nullptr);
    if (udev_monitor_enable_receiving(monitor_) != 0) return;
    auto *notifier = new QSocketNotifier(udev_monitor_get_fd(monitor_), QSocketNotifier::Read, this);
    connect(notifier, &QSocketNotifier::activated, this, [this] {
        while (auto *device = udev_monitor_receive_device(monitor_)) {
            udev_device_unref(device);
            emit changed();
        }
    });
}

HardwareMonitor::~HardwareMonitor()
{
    // Disable notifiers before releasing the descriptor they observe.
    const auto notifiers = findChildren<QSocketNotifier *>();
    for (auto *notifier : notifiers) notifier->setEnabled(false);
    if (monitor_) udev_monitor_unref(monitor_);
    if (context_) udev_unref(context_);
}
