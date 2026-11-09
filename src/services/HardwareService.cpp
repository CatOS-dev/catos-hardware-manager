#include "HardwareService.h"

#include "DeviceDiscovery.h"
#include "ProfileCatalog.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QRegularExpression>
#include <QJsonArray>
#include <QJsonDocument>
#include <algorithm>
#include <QJsonObject>
#include <QFileInfo>
#include <QProcessEnvironment>
#include <QProcess>

namespace {
QStringList strings(const QJsonArray &values)
{
    QStringList result;
    for (const auto &value : values) if (value.isString()) result.append(value.toString());
    return result;
}

DeviceCategory categoryForClass(const QString &code)
{
    if (code.startsWith(QLatin1String("03"))) return DeviceCategory::Graphics;
    if (code.startsWith(QLatin1String("02"))) return DeviceCategory::Network;
    if (code.startsWith(QLatin1String("04"))) return DeviceCategory::Audio;
    return DeviceCategory::Other;
}

DeviceCategory legacyCategory(const QString &name)
{
    if (name.contains(QLatin1String("nvidia"), Qt::CaseInsensitive)
        || QStringList{QStringLiteral("intel"), QStringLiteral("amd"), QStringLiteral("nouveau"),
            QStringLiteral("fallback"), QStringLiteral("qemu"), QStringLiteral("virtualbox"),
            QStringLiteral("vmware")}.contains(name))
        return DeviceCategory::Graphics;
    if (name.contains(QLatin1String("wifi"), Qt::CaseInsensitive)
        || name.contains(QLatin1String("broadcom"), Qt::CaseInsensitive))
        return DeviceCategory::Network;
    if (name.contains(QLatin1String("fprint"), Qt::CaseInsensitive))
        return DeviceCategory::Other;
    return DeviceCategory::Other;
}

QByteArray commandOutput(const QString &program, const QStringList &arguments, QString *error)
{
    QProcess process;
    process.start(program, arguments);
    if (!process.waitForStarted(6000) || !process.waitForFinished(20000)
        || process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        if (process.state() != QProcess::NotRunning) {
            process.kill();
            process.waitForFinished(3000);
        }
        if (error) {
            *error = QString::fromUtf8(process.readAllStandardError()).trimmed();
            if (error->isEmpty()) *error = process.errorString();
        }
        return {};
    }
    return process.readAllStandardOutput();
}

void readService(const QString &name, HardwareSnapshot &snapshot)
{
    QString error;
    const auto output = commandOutput(QStringLiteral("/usr/bin/systemctl"),
        {QStringLiteral("show"), QStringLiteral("--property=UnitFileState,ActiveState"), name}, &error);
    if (output.isEmpty()) {
        snapshot.warnings.append(QCoreApplication::translate("HardwareService",
            "Cannot query service %1: %2").arg(name, error));
        return;
    }
    for (const auto &line : QString::fromUtf8(output).split(QLatin1Char('\n'))) {
        if (line == QLatin1String("ActiveState=active")) snapshot.activeServices.insert(name);
        if (line == QLatin1String("UnitFileState=enabled")
            || line == QLatin1String("UnitFileState=static")
            || line == QLatin1String("UnitFileState=indirect")) snapshot.enabledServices.insert(name);
    }
}

bool unsatisfiedPackages(const QStringList &packages, QSet<QString> &result)
{
    result.clear();
    if (packages.isEmpty()) return true;
    QProcess process;
    process.start(QStringLiteral("/usr/bin/pacman"), QStringList{QStringLiteral("-T"), QStringLiteral("--")} + packages);
    if (!process.waitForStarted(6000) || !process.waitForFinished(20000)
        || process.exitStatus() != QProcess::NormalExit)
        return false;
    for (const auto &name : QString::fromUtf8(process.readAllStandardOutput()).split(QLatin1Char('\n'), Qt::SkipEmptyParts))
        result.insert(name.trimmed());
    return true;
}

void addProfileToDevice(HardwareDevice &device, const QString &profileId)
{
    if (!device.profileIds.contains(profileId)) device.profileIds.append(profileId);
}

HardwareDevice chwdDevice(const QJsonObject &object)
{
    HardwareDevice device;
    device.bus = object.value(QStringLiteral("bus")).toString();
    device.address = object.value(QStringLiteral("id")).toString();
    device.id = device.bus + QLatin1Char(':') + device.address;
    device.name = object.value(QStringLiteral("name")).toString();
    device.vendor = object.value(QStringLiteral("vendor")).toString();
    device.category = categoryForClass(object.value(QStringLiteral("classId")).toString());
    const QStringList available = strings(object.value(QStringLiteral("availableProfiles")).toArray());
    const QStringList installed = strings(object.value(QStringLiteral("installedProfiles")).toArray());
    for (const auto &name : available) addProfileToDevice(device, QStringLiteral("chwd:") + name);
    for (const auto &name : installed) addProfileToDevice(device, QStringLiteral("chwd:") + name);
    const QString recommended = object.value(QStringLiteral("recommendedProfile")).toString();
    if (!recommended.isEmpty()) device.recommendedProfile = QStringLiteral("chwd:") + recommended;
    return device;
}
}

QString categoryTitle(DeviceCategory category)
{
    switch (category) {
    case DeviceCategory::Graphics: return QCoreApplication::translate("HardwareCategory", "Graphics");
    case DeviceCategory::Network: return QCoreApplication::translate("HardwareCategory", "Network");
    case DeviceCategory::Audio: return QCoreApplication::translate("HardwareCategory", "Audio");
    case DeviceCategory::Bluetooth: return QCoreApplication::translate("HardwareCategory", "Bluetooth");
    case DeviceCategory::Camera: return QCoreApplication::translate("HardwareCategory", "Cameras");
    case DeviceCategory::Printer: return QCoreApplication::translate("HardwareCategory", "Printers");
    case DeviceCategory::Scanner: return QCoreApplication::translate("HardwareCategory", "Scanners");
    case DeviceCategory::Tablet: return QCoreApplication::translate("HardwareCategory", "Tablets");
    case DeviceCategory::Fingerprint: return QCoreApplication::translate("HardwareCategory", "Fingerprint readers");
    case DeviceCategory::Virtualization: return QCoreApplication::translate("HardwareCategory", "Virtual machines");
    case DeviceCategory::Power: return QCoreApplication::translate("HardwareCategory", "Power");
    case DeviceCategory::Cpu: return QCoreApplication::translate("HardwareCategory", "Processor");
    case DeviceCategory::Thunderbolt: return QCoreApplication::translate("HardwareCategory", "Thunderbolt");
    case DeviceCategory::Controller: return QCoreApplication::translate("HardwareCategory", "Controllers");
    case DeviceCategory::MobileBroadband: return QCoreApplication::translate("HardwareCategory", "Mobile broadband");
    case DeviceCategory::SmartCard: return QCoreApplication::translate("HardwareCategory", "Smart cards");
    default: return QCoreApplication::translate("HardwareCategory", "Other devices");
    }
}

QString categoryKey(DeviceCategory category)
{
    switch (category) {
    case DeviceCategory::Graphics: return QStringLiteral("graphics");
    case DeviceCategory::Network: return QStringLiteral("network");
    case DeviceCategory::Audio: return QStringLiteral("audio");
    case DeviceCategory::Bluetooth: return QStringLiteral("bluetooth");
    case DeviceCategory::Camera: return QStringLiteral("camera");
    case DeviceCategory::Printer: return QStringLiteral("printer");
    case DeviceCategory::Scanner: return QStringLiteral("scanner");
    case DeviceCategory::Tablet: return QStringLiteral("tablet");
    case DeviceCategory::Fingerprint: return QStringLiteral("fingerprint");
    case DeviceCategory::Virtualization: return QStringLiteral("virtualization");
    case DeviceCategory::Power: return QStringLiteral("power");
    case DeviceCategory::Cpu: return QStringLiteral("cpu");
    case DeviceCategory::Thunderbolt: return QStringLiteral("thunderbolt");
    case DeviceCategory::Controller: return QStringLiteral("controller");
    case DeviceCategory::MobileBroadband: return QStringLiteral("wwan");
    case DeviceCategory::SmartCard: return QStringLiteral("smartcard");
    default: return QStringLiteral("other");
    }
}

QString categoryIcon(DeviceCategory category)
{
    switch (category) {
    case DeviceCategory::Graphics: return QStringLiteral("video-display");
    case DeviceCategory::Network: return QStringLiteral("network-wireless");
    case DeviceCategory::Audio: return QStringLiteral("audio-card");
    case DeviceCategory::Bluetooth: return QStringLiteral("bluetooth");
    case DeviceCategory::Camera: return QStringLiteral("camera-web");
    case DeviceCategory::Printer: return QStringLiteral("printer");
    case DeviceCategory::Scanner: return QStringLiteral("scanner");
    case DeviceCategory::Tablet: return QStringLiteral("input-tablet");
    case DeviceCategory::Fingerprint: return QStringLiteral("fingerprint");
    case DeviceCategory::Virtualization: return QStringLiteral("virtual-machine");
    case DeviceCategory::Power: return QStringLiteral("battery");
    case DeviceCategory::Cpu: return QStringLiteral("cpu");
    case DeviceCategory::Thunderbolt: return QStringLiteral("network-wired");
    case DeviceCategory::Controller: return QStringLiteral("input-gaming");
    case DeviceCategory::MobileBroadband: return QStringLiteral("network-cellular");
    case DeviceCategory::SmartCard: return QStringLiteral("smartcard");
    default: return QStringLiteral("computer");
    }
}

HardwareSnapshot HardwareService::read(const QString &chwdProgram, ReadOptions options)
{
    HardwareSnapshot snapshot;
    QString error;
    const QByteArray raw = commandOutput(chwdProgram,
                                          {QStringLiteral("--json")}, &error);
    const auto document = QJsonDocument::fromJson(raw);
    snapshot.chwdAvailable = document.isObject()
        && document.object().value(QStringLiteral("schema")).toInt() == 1;
    const QJsonObject chwd = snapshot.chwdAvailable ? document.object() : QJsonObject();
    const auto installedChwd = strings(chwd.value(QStringLiteral("installedProfiles")).toArray());
    snapshot.warnings = strings(chwd.value(QStringLiteral("warnings")).toArray());
    if (!snapshot.chwdAvailable)
        snapshot.warnings.append(QCoreApplication::translate("HardwareService",
            "Driver query unavailable: %1").arg(error.isEmpty()
                ? QCoreApplication::translate("HardwareService", "Invalid CHWD response") : error));
    error.clear();

    const QByteArray rawPackages = commandOutput(QStringLiteral("/usr/bin/pacman"),
                                                 {QStringLiteral("-Qq")}, &error);
    if (rawPackages.isEmpty()) {
        snapshot.error = error.isEmpty() ? QStringLiteral("Cannot read installed packages.") : error;
        return snapshot;
    }
    for (const auto &name : QString::fromUtf8(rawPackages).split(QLatin1Char('\n'), Qt::SkipEmptyParts))
        snapshot.installedPackages.insert(name);

    QString catalogError;
    const auto support = ProfileCatalog::load(ProfileCatalog::directoryForApplication(), &catalogError);
    if (!catalogError.isEmpty()) {
        snapshot.error = catalogError;
        return snapshot;
    }
    QSet<QString> acpiHids;
    for (const auto &profile : support)
        for (const auto &rule : profile.matchRules)
            if (!rule.acpiHid.isEmpty()) acpiHids.insert(rule.acpiHid);
    snapshot.devices = DeviceDiscovery::enumerate(options.scanners, acpiHids);
    QHash<QString, int> index;
    for (int i = 0; i < snapshot.devices.size(); ++i)
        index.insert(snapshot.devices[i].id + QLatin1Char(':')
                     + QString::number(static_cast<int>(snapshot.devices[i].category)), i);

    for (const auto &entry : chwd.value(QStringLiteral("devices")).toArray()) {
        if (!entry.isObject()) continue;
        HardwareDevice device = chwdDevice(entry.toObject());
        if (device.profileIds.isEmpty() && device.category == DeviceCategory::Other) continue;
        const QString key = device.id + QLatin1Char(':')
                            + QString::number(static_cast<int>(device.category));
        if (index.contains(key)) {
            auto &existing = snapshot.devices[index.value(key)];
            for (const auto &profile : device.profileIds) addProfileToDevice(existing, profile);
            if (!device.recommendedProfile.isEmpty()) existing.recommendedProfile = device.recommendedProfile;
            if (existing.name.isEmpty() || existing.name == existing.address) existing.name = device.name;
            if (existing.vendor.isEmpty()) existing.vendor = device.vendor;
        } else {
            index.insert(key, snapshot.devices.size());
            snapshot.devices.append(std::move(device));
        }
    }

    for (const auto &entry : chwd.value(QStringLiteral("profiles")).toArray()) {
        if (!entry.isObject()) continue;
        const QJsonObject item = entry.toObject();
        const QString name = item.value(QStringLiteral("name")).toString();
        if (name.isEmpty()) continue;
        HardwareProfile profile;
        profile.source = ProfileSource::Chwd;
        profile.id = QStringLiteral("chwd:") + name;
        profile.chwdName = name;
        profile.name = name;
        profile.description = item.value(QStringLiteral("description")).toString();
        if (name == QLatin1String("fallback")) {
            profile.name = QStringLiteral("Mesa + Software Vulkan + VESA");
            profile.chineseName = QStringLiteral("Mesa + 软件 Vulkan + VESA");
            profile.description = QStringLiteral(
                "Mesa (OpenGL), vulkan-swrast (software Vulkan), xf86-video-vesa (VESA driver)");
            profile.chineseDescription = QStringLiteral(
                "Mesa（OpenGL）、vulkan-swrast（软件 Vulkan）、xf86-video-vesa（VESA 驱动）");
            profile.traditionalChineseName = QStringLiteral("Mesa + 軟體 Vulkan + VESA");
            profile.traditionalChineseDescription = QStringLiteral(
                "Mesa（OpenGL）、vulkan-swrast（軟體 Vulkan）、xf86-video-vesa（VESA 驅動程式）");
        }
        profile.packages = strings(item.value(QStringLiteral("packages")).toArray());
        profile.conflictGroup = item.value(QStringLiteral("conflictGroup")).toString();
        profile.installed = installedChwd.contains(name);
        profile.category = legacyCategory(name);
        for (const auto &device : snapshot.devices) {
            if (device.profileIds.contains(profile.id)) {
                profile.applicable = true;
                profile.category = device.category;
                break;
            }
        }
        snapshot.profiles.append(profile);
    }

    // Profile rules are completely independent of CHWD and live in a
    // separate, declarative catalog.
    error.clear();
    QStringList packageNames;
    for (const auto &profile : support)
        for (const auto &package : profile.packages)
            if (!packageNames.contains(package)) packageNames.append(package);
    snapshot.packageSatisfiabilityKnown = unsatisfiedPackages(packageNames, snapshot.unsatisfiedPackages);
    if (!snapshot.packageSatisfiabilityKnown)
        snapshot.warnings.append(QCoreApplication::translate("HardwareService",
            "Cannot query package dependency satisfaction; using installed package names."));
    // Only services of profiles that apply to this machine are relevant;
    // querying guest-tool services on bare metal only produces noise.
    QSet<QString> serviceNames;
    for (const auto &profile : support) {
        const bool applicable = profile.discovery
            || std::any_of(snapshot.devices.cbegin(), snapshot.devices.cend(),
                           [&profile](const HardwareDevice &device) { return ProfileCatalog::matches(profile, device); });
        if (!applicable) continue;
        for (const auto &service : profile.services) serviceNames.insert(service);
    }
    for (const auto &service : serviceNames)
        readService(service, snapshot);

    attachSupportProfiles(snapshot, support);
    for (const auto &device : snapshot.devices) {
        if (device.category != DeviceCategory::Cpu) continue;
        const QString package = device.capabilities.contains(QStringLiteral("cpu-amd"))
            ? QStringLiteral("amd-ucode") : QStringLiteral("intel-ucode");
        if (snapshot.installedPackages.contains(package)
            && microcodeBoot(QStringLiteral("/"), package) == MicrocodeBoot::NotLoaded)
            snapshot.warnings.append(QCoreApplication::translate("HardwareService",
                "%1 is installed, but neither the initramfs nor the boot loader entries load it. Add the microcode hook to mkinitcpio or reference /boot/%1.img in the boot entry.").arg(package));
        break;
    }
    snapshot.devices.erase(std::remove_if(snapshot.devices.begin(), snapshot.devices.end(),
        [](const HardwareDevice &device) {
            return device.category == DeviceCategory::Other && device.profileIds.isEmpty();
        }), snapshot.devices.end());
    std::sort(snapshot.devices.begin(), snapshot.devices.end(), [](const auto &a, const auto &b) {
        if (a.category != b.category) return a.category < b.category;
        return a.name.localeAwareCompare(b.name) < 0;
    });
    return snapshot;
}

MicrocodeBoot HardwareService::microcodeBoot(const QString &root, const QString &package)
{
    const QDir base(root);
    const auto read = [&base](const QString &path) {
        QFile file(base.filePath(path));
        return file.open(QIODevice::ReadOnly) ? QString::fromUtf8(file.readAll()) : QString();
    };
    // mkinitcpio 38+ embeds microcode through the "microcode" hook; dracut
    // and booster embed it by default.
    const bool mkinitcpio = QFileInfo::exists(base.filePath(QStringLiteral("usr/bin/mkinitcpio")));
    if (!mkinitcpio) {
        if (QFileInfo::exists(base.filePath(QStringLiteral("usr/bin/dracut")))
            || QFileInfo::exists(base.filePath(QStringLiteral("usr/bin/booster"))))
            return MicrocodeBoot::Loaded;
        return MicrocodeBoot::Unknown;
    }
    QStringList mkinitcpioConfigs{read(QStringLiteral("etc/mkinitcpio.conf"))};
    const QDir dropins(base.filePath(QStringLiteral("etc/mkinitcpio.conf.d")));
    for (const auto &name : dropins.entryList({QStringLiteral("*.conf")}, QDir::Files))
        mkinitcpioConfigs.append(read(QStringLiteral("etc/mkinitcpio.conf.d/") + name));
    const QRegularExpression hooks(QStringLiteral("^\\s*HOOKS\\s*=\\s*\\(([^)]*)\\)"), QRegularExpression::MultilineOption);
    QString activeHooks;
    for (const auto &config : mkinitcpioConfigs) {
        auto matches = hooks.globalMatch(config);
        while (matches.hasNext()) activeHooks = matches.next().captured(1);  // last assignment wins
    }
    if (activeHooks.split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts)
            .contains(QStringLiteral("microcode")))
        return MicrocodeBoot::Loaded;
    if (activeHooks.isEmpty()) return MicrocodeBoot::Unknown;

    QStringList entries{QStringLiteral("boot/grub/grub.cfg"), QStringLiteral("boot/limine.conf"),
        QStringLiteral("boot/limine/limine.conf"), QStringLiteral("boot/EFI/limine/limine.conf"),
        QStringLiteral("efi/limine.conf"), QStringLiteral("efi/EFI/limine/limine.conf"),
        QStringLiteral("boot/efi/limine.conf"), QStringLiteral("boot/refind_linux.conf")};
    for (const auto &directory : {QStringLiteral("boot/loader/entries"), QStringLiteral("efi/loader/entries"),
                                  QStringLiteral("boot/efi/loader/entries")}) {
        for (const auto &name : QDir(base.filePath(directory)).entryList({QStringLiteral("*.conf")}, QDir::Files))
            entries.append(directory + QLatin1Char('/') + name);
    }
    bool sawEntry = false;
    for (const auto &entry : entries) {
        const QString contents = read(entry);
        if (contents.isEmpty()) continue;
        sawEntry = true;
        if (contents.contains(package + QStringLiteral(".img"))) return MicrocodeBoot::Loaded;
    }
    // Unreadable /boot or a UKI-only setup: no evidence either way.
    return sawEntry ? MicrocodeBoot::NotLoaded : MicrocodeBoot::Unknown;
}

void HardwareService::updateSupportState(HardwareProfile &profile, const HardwareSnapshot &snapshot)
{
    profile.missingPackages.clear();
    profile.conflictingPackages.clear();
    profile.disabledServices.clear();
    profile.inactiveServices.clear();
    for (const auto &name : profile.packages) {
        const bool missing = snapshot.packageSatisfiabilityKnown
            ? snapshot.unsatisfiedPackages.contains(name)
            : !snapshot.installedPackages.contains(name);
        if (missing) profile.missingPackages.append(name);
    }
    for (const auto &name : profile.conflicts)
        if (snapshot.installedPackages.contains(name)) profile.conflictingPackages.append(name);
    for (const auto &name : profile.services) {
        if (!snapshot.enabledServices.contains(name)) profile.disabledServices.append(name);
        if (!snapshot.activeServices.contains(name)) profile.inactiveServices.append(name);
    }
    profile.installed = profile.conflictingPackages.isEmpty()
        && profile.missingPackages.isEmpty() && profile.disabledServices.isEmpty();
    profile.ready = profile.installed && profile.inactiveServices.isEmpty();
}

void HardwareService::attachSupportProfiles(HardwareSnapshot &snapshot, const QVector<HardwareProfile> &support)
{
    for (auto profile : support) {
        profile.applicable = profile.discovery;
        for (auto &device : snapshot.devices) {
            if (ProfileCatalog::matches(profile, device)) {
                profile.applicable = true;
                addProfileToDevice(device, profile.id);
                if (device.category == DeviceCategory::Other)
                    device.category = profile.category;
            }
        }
        updateSupportState(profile, snapshot);
        profile.systemManaged = true;
        snapshot.profiles.append(std::move(profile));
    }
}
