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
            const QString action = QString::fromUtf8(udev_device_get_action(device));
            const QString subsystem = QString::fromUtf8(udev_device_get_subsystem(device));
            const QString syspath = QString::fromUtf8(udev_device_get_syspath(device));
            const bool relevantAction = action == QLatin1String("add")
                || action == QLatin1String("remove") || action == QLatin1String("bind")
                || action == QLatin1String("unbind");
            bool relevant = relevantAction;
            if (subsystem == QLatin1String("net")) {
                relevant = relevantAction && !syspath.startsWith(QLatin1String("/sys/devices/virtual/"))
                    && (udev_device_get_parent_with_subsystem_devtype(device, "usb", "usb_device")
                        || udev_device_get_parent_with_subsystem_devtype(device, "pci", nullptr));
            }
            udev_device_unref(device);
            if (relevant) emit changed();
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
