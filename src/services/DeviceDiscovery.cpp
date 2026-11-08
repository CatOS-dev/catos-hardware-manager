#include "DeviceDiscovery.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QRegularExpression>
#include <QProcess>
#include <QProcessEnvironment>
#include <QStandardPaths>
#include <libudev.h>

#include <algorithm>
#include <memory>

namespace {
QString text(const char *value)
{
    return value ? QString::fromUtf8(value) : QString();
}

QString property(udev_device *dev, const char *name)
{
    return text(udev_device_get_property_value(dev, name));
}

QString attribute(udev_device *dev, const char *name)
{
    return text(udev_device_get_sysattr_value(dev, name));
}

udev_device *physicalDevice(udev_device *dev)
{
    const QString subsystem = text(udev_device_get_subsystem(dev));
    const QString devtype = text(udev_device_get_devtype(dev));
    if (subsystem == QLatin1String("pci")
        || (subsystem == QLatin1String("usb") && devtype == QLatin1String("usb_device"))
        || subsystem == QLatin1String("hid") || subsystem == QLatin1String("i2c"))
        return dev;
    if (auto *p = udev_device_get_parent_with_subsystem_devtype(dev, "hid", nullptr))
        return p;
    if (auto *p = udev_device_get_parent_with_subsystem_devtype(dev, "i2c", nullptr))
        return p;
    if (auto *p = udev_device_get_parent_with_subsystem_devtype(dev, "usb", "usb_device"))
        return p;
    return udev_device_get_parent_with_subsystem_devtype(dev, "pci", nullptr);
}

udev_device *parent(udev_device *dev)
{
    return physicalDevice(dev);
}

QStringList modulesForModalias(const QString &modalias)
{
    static QHash<QString, QStringList> cache;
    if (cache.contains(modalias)) return cache.value(modalias);
    QStringList result;
    QProcess uname;
    uname.start(QStringLiteral("uname"), {QStringLiteral("-r")});
    if (uname.waitForFinished(1000) && uname.exitCode() == 0) {
        QFile aliases(QStringLiteral("/lib/modules/")
                      + QString::fromUtf8(uname.readAllStandardOutput()).trimmed()
                      + QStringLiteral("/modules.alias"));
        if (aliases.open(QIODevice::ReadOnly | QIODevice::Text)) {
            const auto lines = QString::fromUtf8(aliases.readAll()).split(QLatin1Char('\n'));
            for (const auto &line : lines) {
                if (!line.startsWith(QLatin1String("alias "))) continue;
                const auto fields = line.split(QLatin1Char(' '), Qt::SkipEmptyParts);
                if (fields.size() != 3) continue;
                QString pattern = QRegularExpression::escape(fields[1]);
                pattern.replace(QStringLiteral("\\*"), QStringLiteral(".*"));
                pattern.replace(QStringLiteral("\\?"), QStringLiteral("."));
                if (QRegularExpression(QStringLiteral("^") + pattern + QStringLiteral("$"),
                                        QRegularExpression::CaseInsensitiveOption).match(modalias).hasMatch()
                    && !result.contains(fields[2])) result.append(fields[2]);
            }
        }
    }
    cache.insert(modalias, result);
    return result;
}

QString preferred(const QStringList &values)
{
    for (const auto &value : values) {
        if (!value.isEmpty()) return value;
    }
    return {};
}

QString driverName(udev_device *dev)
{
    if (const char *driver = udev_device_get_driver(dev)) return text(driver);
    const QFileInfo symlink(text(udev_device_get_syspath(dev)) + QStringLiteral("/driver"));
    if (symlink.isSymLink()) return QFileInfo(symlink.symLinkTarget()).fileName();
    return {};
}

QString boundDriver(udev_device *dev)
{
    // The driver belongs to the functional device being reported. Walking
    // toward a hub or PCI bridge reports the controller driver instead.
    return driverName(dev);
}

QString physicalKey(udev_device *dev)
{
    auto *physical = physicalDevice(dev);
    if (!physical) return text(udev_device_get_syspath(dev));
    const QString subsystem = text(udev_device_get_subsystem(physical));
    if (subsystem == QLatin1String("usb"))
        return QStringLiteral("usb:") + text(udev_device_get_sysname(physical));
    if (subsystem == QLatin1String("pci"))
        return QStringLiteral("pci:") + text(udev_device_get_sysname(physical));
    return text(udev_device_get_syspath(physical));
}

HardwareDevice makeDevice(udev_device *dev, DeviceCategory category,
                          const QString &capability)
{
    auto *physical = parent(dev);
    if (!physical) physical = dev;
    HardwareDevice result;
    result.id = physicalKey(dev);
    result.category = category;
    result.address = text(udev_device_get_sysname(physical));
    result.bus = text(udev_device_get_subsystem(physical));
    result.modalias = preferred({property(dev, "MODALIAS"), attribute(dev, "modalias"),
                                property(physical, "MODALIAS"), attribute(physical, "modalias")});
    result.driver = boundDriver(dev);
    result.modules = modulesForModalias(result.modalias);
    result.name = preferred({property(dev, "ID_MODEL_FROM_DATABASE"), property(physical, "ID_MODEL_FROM_DATABASE"),
                             property(dev, "ID_MODEL"), property(physical, "ID_MODEL"),
                             attribute(physical, "product"), attribute(physical, "device"),
                             result.address});
    result.vendor = preferred({property(dev, "ID_VENDOR_FROM_DATABASE"),
                               property(physical, "ID_VENDOR_FROM_DATABASE"),
                               property(dev, "ID_VENDOR"), property(physical, "ID_VENDOR"),
                               attribute(physical, "manufacturer"), attribute(physical, "vendor")});
    result.vendorId = preferred({property(physical, "ID_VENDOR_ID"), attribute(physical, "idVendor"),
                                 attribute(physical, "vendor")});
    result.productId = preferred({property(physical, "ID_MODEL_ID"), attribute(physical, "idProduct"),
                                  attribute(physical, "device")});
    if (text(udev_device_get_subsystem(physical)) == QLatin1String("hid")) {
        const QString hidId = property(physical, "HID_ID");
        const auto parts = hidId.split(QLatin1Char(':'));
        if (parts.size() >= 3) {
            result.vendorId = parts.at(parts.size() - 2);
            result.productId = parts.at(parts.size() - 1);
        }
        result.name = preferred({property(physical, "HID_NAME"), result.name});
        result.bus = QStringLiteral("hid");
    }
    result.acpiHid = attribute(physical, "hid");
    if (result.acpiHid.isEmpty()
        && result.bus == QLatin1String("acpi"))
        result.acpiHid = result.address.section(QLatin1Char(':'), 0, 0);
    result.capabilities.append(capability);
    return result;
}

void add(QHash<QString, HardwareDevice> &all, HardwareDevice device)
{
    if (device.name.isEmpty()) return;
    // A composite device can expose several functional interfaces. A physical
    // device is shown once per category, not once per kernel interface.
    const QString key = QString::number(static_cast<int>(device.category))
                        + QLatin1Char(':') + device.id;
    if (auto found = all.find(key); found != all.end()) {
        for (const auto &cap : device.capabilities) {
            if (!found->capabilities.contains(cap)) found->capabilities.append(cap);
        }
        if (found->driver.isEmpty()) found->driver = device.driver;
        return;
    }
    all.insert(key, std::move(device));
}

bool inputProperty(udev_device *dev, const char *name)
{
    return property(dev, name) == QLatin1String("1");
}

QString sysName(udev_device *dev)
{
    return text(udev_device_get_sysname(dev));
}

bool unsupportedFingerprint(const QString &vendor, const QString &product)
{
    static QStringList ids;
    static bool loaded = false;
    if (!loaded) {
        loaded = true;
        QFile file(QStringLiteral("/usr/share/catos-hardware-manager/fingerprint-unsupported.txt"));
        bool opened = file.open(QIODevice::ReadOnly);
        if (!opened) {
            file.setFileName(QCoreApplication::applicationDirPath()
                             + QStringLiteral("/../data/fingerprint-unsupported.txt"));
            opened = file.open(QIODevice::ReadOnly);
        }
        if (opened)
            ids = QString::fromUtf8(file.readAll()).split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    }
    const QString id = vendor.toLower() + QLatin1Char(':') + product.toLower();
    return std::any_of(ids.begin(), ids.end(), [&id](const QString &line) {
        const QString value = line.trimmed();
        return !value.startsWith(QLatin1Char('#')) && value.compare(id, Qt::CaseInsensitive) == 0;
    });
}

bool matchesInterface(udev_device *dev, const char *classId)
{
    return attribute(dev, "bInterfaceClass").compare(QLatin1String(classId), Qt::CaseInsensitive) == 0;
}

QVector<HardwareDevice> enumerateSubsystem(udev *context, const char *subsystem,
                                            const QSet<QString> &acpiHids)
{
    QVector<HardwareDevice> devices;
    std::unique_ptr<udev_enumerate, decltype(&udev_enumerate_unref)>
        enumerator(udev_enumerate_new(context), &udev_enumerate_unref);
    if (!enumerator) return devices;
    udev_enumerate_add_match_subsystem(enumerator.get(), subsystem);
    if (udev_enumerate_scan_devices(enumerator.get()) != 0) return devices;
    for (auto *entry = udev_enumerate_get_list_entry(enumerator.get());
         entry; entry = udev_list_entry_get_next(entry)) {
        const char *path = udev_list_entry_get_name(entry);
        if (!path) continue;
        std::unique_ptr<udev_device, decltype(&udev_device_unref)>
            device(udev_device_new_from_syspath(context, path), &udev_device_unref);
        if (!device) continue;
        const QString sub = QLatin1String(subsystem);
        const QString name = sysName(device.get());
        if (sub == QLatin1String("pci")) {
            const QString classId = attribute(device.get(), "class").toLower();
            if (classId.startsWith(QLatin1String("0x03")))
                devices.append(makeDevice(device.get(), DeviceCategory::Graphics, QStringLiteral("graphics")));
            else if (classId.startsWith(QLatin1String("0x02")))
                devices.append(makeDevice(device.get(), DeviceCategory::Network, QStringLiteral("network")));
            else if (classId.startsWith(QLatin1String("0x0401"))
                     || classId.startsWith(QLatin1String("0x0403")))
                devices.append(makeDevice(device.get(), DeviceCategory::Audio, QStringLiteral("audio")));
        } else if (sub == QLatin1String("sound")) {
            if (QRegularExpression(QStringLiteral("^card[0-9]+$")).match(name).hasMatch())
                devices.append(makeDevice(device.get(), DeviceCategory::Audio, QStringLiteral("audio")));
        } else if (sub == QLatin1String("bluetooth")) {
            if (QRegularExpression(QStringLiteral("^hci[0-9]+$")).match(name).hasMatch())
                devices.append(makeDevice(device.get(), DeviceCategory::Bluetooth, QStringLiteral("bluetooth")));
        } else if (sub == QLatin1String("video4linux")) {
            const QString caps = property(device.get(), "ID_V4L_CAPABILITIES");
            if (caps.contains(QLatin1String(":capture:"))) {
                QString cap = QStringLiteral("camera-v4l2");
                if (auto *deviceParent = parent(device.get())) {
                    if (text(udev_device_get_subsystem(deviceParent)) == QLatin1String("usb"))
                        cap = QStringLiteral("camera-uvc");
                }
                if (cap == QLatin1String("camera-v4l2")
                    && udev_device_get_parent_with_subsystem_devtype(device.get(), "platform", nullptr))
                    cap = QStringLiteral("camera-mipi");
                devices.append(makeDevice(device.get(), DeviceCategory::Camera, cap));
            }
        } else if (sub == QLatin1String("input")) {
            if (inputProperty(device.get(), "ID_INPUT_TABLET") || inputProperty(device.get(), "ID_INPUT_TABLET_PAD"))
                devices.append(makeDevice(device.get(), DeviceCategory::Tablet, QStringLiteral("tablet")));
            if (inputProperty(device.get(), "ID_INPUT_JOYSTICK"))
                devices.append(makeDevice(device.get(), DeviceCategory::Controller, QStringLiteral("gamepad")));
        } else if (sub == QLatin1String("thunderbolt")) {
            if (!name.contains(QLatin1Char(':'))) {
                auto controller = makeDevice(device.get(), DeviceCategory::Thunderbolt,
                                             QStringLiteral("thunderbolt"));
                if (name.startsWith(QLatin1String("domain")))
                    controller.name = QCoreApplication::translate("DeviceDiscovery", "Thunderbolt / USB4 controller");
                devices.append(std::move(controller));
            }
        } else if (sub == QLatin1String("usb")) {
            const QString devtype = text(udev_device_get_devtype(device.get()));
            if (devtype == QLatin1String("usb_device")) {
                if (attribute(device.get(), "bDeviceClass").compare(QLatin1String("09"), Qt::CaseInsensitive) != 0) {
                    auto generic = makeDevice(device.get(), DeviceCategory::Other, QStringLiteral("usb-device"));
                    if (unsupportedFingerprint(generic.vendorId, generic.productId)) {
                        generic.category = DeviceCategory::Fingerprint;
                        generic.capabilities.append(QStringLiteral("fingerprint-unsupported"));
                    }
                    devices.append(std::move(generic));
                }
                continue;
            }
            if (devtype != QLatin1String("usb_interface")) continue;
            if (matchesInterface(device.get(), "07"))
                devices.append(makeDevice(device.get(), DeviceCategory::Printer, QStringLiteral("usb-printer")));
            if (matchesInterface(device.get(), "0b"))
                devices.append(makeDevice(device.get(), DeviceCategory::SmartCard, QStringLiteral("smartcard")));
        } else if (sub == QLatin1String("acpi")) {
            const QString acpiId = attribute(device.get(), "hid");
            if (!acpiId.isEmpty() && acpiHids.contains(acpiId))
                devices.append(makeDevice(device.get(), DeviceCategory::Other, QStringLiteral("acpi")));
        } else if (sub == QLatin1String("wwan")) {
            devices.append(makeDevice(device.get(), DeviceCategory::MobileBroadband, QStringLiteral("wwan")));
        } else if (sub == QLatin1String("net")) {
            // PCI network controllers have already been collected above.
            // Only include actual USB network interfaces, not software interfaces.
            if (auto *usb = udev_device_get_parent_with_subsystem_devtype(device.get(), "usb", "usb_device")) {
                Q_UNUSED(usb);
                devices.append(makeDevice(device.get(), DeviceCategory::Network, QStringLiteral("network")));
            }
            const QString driver = boundDriver(device.get());
            if (QStringList{QStringLiteral("qmi_wwan"), QStringLiteral("cdc_mbim"),
                            QStringLiteral("mhi_wwan_ctrl")}.contains(driver))
                devices.append(makeDevice(device.get(), DeviceCategory::MobileBroadband, QStringLiteral("wwan")));
        }
    }
    return devices;
}
}

QVector<HardwareDevice> DeviceDiscovery::enumerate(bool scanners)
{
    return enumerate(scanners, {});
}

QVector<HardwareDevice> DeviceDiscovery::enumerate(bool scanners, const QSet<QString> &acpiHids)
{
    std::unique_ptr<udev, decltype(&udev_unref)> context(udev_new(), &udev_unref);
    if (!context) return {};
    QHash<QString, HardwareDevice> all;
    for (const char *subsystem : {"pci", "sound", "bluetooth", "video4linux", "input",
                                  "thunderbolt", "usb", "net", "wwan", "acpi"}) {
        for (auto device : enumerateSubsystem(context.get(), subsystem, acpiHids))
            add(all, std::move(device));
    }
    // SANE can identify USB/network scanners for which no standard USB
    // interface class exists. Do not misclassify all imaging-class devices.
    if (scanners) {
      QProcess scan;
    auto environment = QProcessEnvironment::systemEnvironment();
    environment.insert(QStringLiteral("LC_ALL"), QStringLiteral("C"));
    scan.setProcessEnvironment(environment);
    scan.start(QStringLiteral("scanimage"), {QStringLiteral("-L")});
    const bool scannerStarted = scan.waitForStarted(1200);
    const bool scannerFinished = scannerStarted && scan.waitForFinished(3500);
    if (scannerStarted && !scannerFinished) {
        scan.kill();
        scan.waitForFinished();
    }
      if (scannerFinished && scan.exitStatus() == QProcess::NormalExit && scan.exitCode() == 0) {
        const QString output = QString::fromUtf8(scan.readAllStandardOutput());
        const QRegularExpression pattern(QStringLiteral("^device ['`][^']+' is a (.+)$"),
                                         QRegularExpression::MultilineOption);
        auto matches = pattern.globalMatch(output);
        while (matches.hasNext()) {
            const auto match = matches.next();
            const QString line = match.captured(0);
            if (line.contains(QStringLiteral("v4l:"), Qt::CaseInsensitive)) continue;
            HardwareDevice scanner;
            scanner.id = QStringLiteral("sane:") + line;
            scanner.name = match.captured(1);
            scanner.address = line.section(QLatin1Char('\''), 1, 1);
            scanner.bus = QStringLiteral("sane");
            scanner.category = DeviceCategory::Scanner;
            scanner.capabilities.append(QStringLiteral("scanner"));
            add(all, std::move(scanner));
        }
      }
    }
    int graphicsCount = 0;
    for (auto it = all.cbegin(); it != all.cend(); ++it)
        if (it->category == DeviceCategory::Graphics) ++graphicsCount;
    if (graphicsCount > 1) {
        HardwareDevice device;
        device.id = QStringLiteral("system:graphics");
        device.category = DeviceCategory::Graphics;
        device.bus = QStringLiteral("system");
        device.address = QStringLiteral("graphics");
        device.name = QStringLiteral("Multiple graphics adapters");
        device.capabilities.append(QStringLiteral("dual-gpu"));
        if (QFileInfo::exists(QStringLiteral("/sys/kernel/debug/vgaswitcheroo/switch"))
            || !QStandardPaths::findExecutable(QStringLiteral("switcherooctl")).isEmpty())
            device.capabilities.append(QStringLiteral("switcheroo"));
        add(all, std::move(device));
    }

    QFile cpuInfo(QStringLiteral("/proc/cpuinfo"));
    if (cpuInfo.open(QIODevice::ReadOnly | QIODevice::Text)) {
        const QString contents = QString::fromUtf8(cpuInfo.readAll());
        const auto match = QRegularExpression(QStringLiteral("^vendor_id\\s*:\\s*(.+)$"), QRegularExpression::MultilineOption).match(contents);
        const QString vendor = match.captured(1).trimmed().toLower();
        const QString capability = vendor == QLatin1String("authenticamd") ? QStringLiteral("cpu-amd")
            : vendor == QLatin1String("genuineintel") ? QStringLiteral("cpu-intel") : QString();
        if (!capability.isEmpty()) {
            HardwareDevice device;
            device.id = QStringLiteral("system:cpu");
            device.category = DeviceCategory::Cpu;
            device.bus = QStringLiteral("system");
            device.address = QStringLiteral("cpu");
            device.name = vendor == QLatin1String("authenticamd") ? QStringLiteral("AMD processor") : QStringLiteral("Intel processor");
            device.capabilities.append(capability);
            add(all, std::move(device));
        }
    }

    QProcess virtualization;
    virtualization.start(QStringLiteral("systemd-detect-virt"), {QStringLiteral("--vm")});
    if (virtualization.waitForFinished(1200) && virtualization.exitStatus() == QProcess::NormalExit
        && virtualization.exitCode() == 0) {
        const QString type = QString::fromUtf8(virtualization.readAllStandardOutput()).trimmed().toLower();
        if (!type.isEmpty()) {
            HardwareDevice device;
            device.id = QStringLiteral("system:virt");
            device.category = DeviceCategory::Virtualization;
            device.bus = QStringLiteral("system");
            device.address = QStringLiteral("virt");
            device.name = type;
            device.capabilities.append(QStringLiteral("virt-") + type);
            if (QFileInfo::exists(QStringLiteral("/dev/virtio-ports/org.qemu.guest_agent.0")))
                device.capabilities.append(QStringLiteral("qemu-ga-port"));
            if (QFileInfo::exists(QStringLiteral("/dev/virtio-ports/com.redhat.spice.0")))
                device.capabilities.append(QStringLiteral("spice-port"));
            add(all, std::move(device));
        }
    }
    QDir powerDirectory(QStringLiteral("/sys/class/power_supply"));
    for (const auto &entry : powerDirectory.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        QFile typeFile(powerDirectory.filePath(entry + QStringLiteral("/type")));
        QFile scopeFile(powerDirectory.filePath(entry + QStringLiteral("/scope")));
        if (!typeFile.open(QIODevice::ReadOnly) || QString::fromUtf8(typeFile.readAll()).trimmed() != QLatin1String("Battery")) continue;
        QString scope;
        if (scopeFile.open(QIODevice::ReadOnly)) scope = QString::fromUtf8(scopeFile.readAll()).trimmed();
        if (scope.compare(QLatin1String("Device"), Qt::CaseInsensitive) == 0) continue;
        HardwareDevice device;
        device.id = QStringLiteral("system:power");
        device.category = DeviceCategory::Power;
        device.bus = QStringLiteral("power_supply");
        device.address = entry;
        device.name = QStringLiteral("Battery");
        device.capabilities.append(QStringLiteral("battery"));
        add(all, std::move(device));
        break;
    }

    QVector<HardwareDevice> result;
    QFile vendor(QStringLiteral("/sys/class/dmi/id/sys_vendor"));
    QString vendorName;
    if (vendor.open(QIODevice::ReadOnly)) vendorName = QString::fromUtf8(vendor.readAll()).trimmed();
    for (auto device : all) {
        device.dmiVendor = vendorName;
        result.append(std::move(device));
    }
    std::sort(result.begin(), result.end(), [](const auto &a, const auto &b) {
        if (a.category != b.category) return a.category < b.category;
        return a.name.localeAwareCompare(b.name) < 0;
    });
    return result;
}
