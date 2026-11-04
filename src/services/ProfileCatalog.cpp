#include "ProfileCatalog.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QRegularExpression>
#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <stdexcept>

namespace {
QString field(const YAML::Node &node, const char *name)
{
    return node[name] && node[name].IsScalar()
        ? QString::fromStdString(node[name].as<std::string>()) : QString();
}

QStringList list(const YAML::Node &node)
{
    QStringList result;
    if (!node || !node.IsSequence()) return result;
    for (const auto &entry : node) {
        if (!entry.IsScalar()) throw std::runtime_error("Non-string list entry");
        result.append(QString::fromStdString(entry.as<std::string>()));
    }
    return result;
}

DeviceCategory category(const QString &name)
{
    static const QHash<QString, DeviceCategory> categories{
        {QStringLiteral("graphics"), DeviceCategory::Graphics},
        {QStringLiteral("network"), DeviceCategory::Network},
        {QStringLiteral("audio"), DeviceCategory::Audio},
        {QStringLiteral("bluetooth"), DeviceCategory::Bluetooth},
        {QStringLiteral("camera"), DeviceCategory::Camera},
        {QStringLiteral("printer"), DeviceCategory::Printer},
        {QStringLiteral("scanner"), DeviceCategory::Scanner},
        {QStringLiteral("tablet"), DeviceCategory::Tablet},
        {QStringLiteral("firmware"), DeviceCategory::Firmware},
        {QStringLiteral("thunderbolt"), DeviceCategory::Thunderbolt},
        {QStringLiteral("controller"), DeviceCategory::Controller},
        {QStringLiteral("wwan"), DeviceCategory::MobileBroadband},
        {QStringLiteral("smartcard"), DeviceCategory::SmartCard}
    };
    return categories.value(name, DeviceCategory::Other);
}

bool validPackage(const QString &value)
{
    static const QRegularExpression pattern(QStringLiteral("^[a-z0-9@][a-zA-Z0-9@._+-]{0,127}$"));
    return pattern.match(value).hasMatch();
}

bool validService(const QString &value)
{
    static const QRegularExpression pattern(QStringLiteral("^[a-zA-Z0-9@_.:-]{1,128}\\.(service|socket)$"));
    return pattern.match(value).hasMatch();
}

bool sameId(QString left, QString right)
{
    left.remove(QStringLiteral("0x"), Qt::CaseInsensitive);
    right.remove(QStringLiteral("0x"), Qt::CaseInsensitive);
    return left.compare(right, Qt::CaseInsensitive) == 0;
}

HardwareMatchRule readMatch(const YAML::Node &node)
{
    if (!node.IsMap() || node.size() == 0) throw std::runtime_error("Empty match rule");
    HardwareMatchRule rule;
    for (const auto &part : node) {
        if (!part.first.IsScalar() || !part.second.IsScalar())
            throw std::runtime_error("Invalid match field");
        const QString key = QString::fromStdString(part.first.as<std::string>());
        const QString value = QString::fromStdString(part.second.as<std::string>());
        if (value.isEmpty()) throw std::runtime_error("Empty match value");
        if (key == QLatin1String("capability")) rule.capability = value;
        else if (key == QLatin1String("bus")) rule.bus = value;
        else if (key == QLatin1String("vendor")) rule.vendor = value;
        else if (key == QLatin1String("device")) rule.device = value;
        else if (key == QLatin1String("driver")) rule.driver = value;
        else if (key == QLatin1String("modalias_prefix")) rule.modaliasPrefix = value;
        else if (key == QLatin1String("dmi_vendor")) rule.dmiVendor = value;
        else if (key == QLatin1String("acpi_hid")) rule.acpiHid = value;
        else throw std::runtime_error("Unknown match predicate");
    }
    return rule;
}

bool matchesRule(const HardwareMatchRule &rule, const HardwareDevice &device)
{
    if (!rule.capability.isEmpty() && !device.capabilities.contains(rule.capability)) return false;
    if (!rule.bus.isEmpty() && rule.bus != device.bus) return false;
    if (!rule.vendor.isEmpty() && !sameId(rule.vendor, device.vendorId)) return false;
    if (!rule.device.isEmpty() && !sameId(rule.device, device.productId)) return false;
    if (!rule.driver.isEmpty() && rule.driver != device.driver) return false;
    if (!rule.modaliasPrefix.isEmpty()
        && !device.modalias.startsWith(rule.modaliasPrefix, Qt::CaseInsensitive)) return false;
    if (!rule.dmiVendor.isEmpty() && rule.dmiVendor != device.dmiVendor) return false;
    if (!rule.acpiHid.isEmpty() && rule.acpiHid != device.acpiHid) return false;
    return true;
}
}

QString ProfileCatalog::installedDirectory()
{
    return QStringLiteral("/usr/share/catos-hardware-manager/profiles");
}

QString ProfileCatalog::directoryForApplication()
{
    const QDir directory(QCoreApplication::applicationDirPath());
    const QString profiles = directory.filePath(QStringLiteral("../data/profiles"));
    if (QFile::exists(directory.filePath(QStringLiteral("../CMakeLists.txt")))
        && QDir(profiles).exists()) return QDir(profiles).absolutePath();
    return installedDirectory();
}

QVector<HardwareProfile> ProfileCatalog::load(const QString &directory, QString *error)
{
    if (error) error->clear();
    QVector<HardwareProfile> result;
    QSet<QString> ids;
    const QDir path(directory);
    if (!path.exists()) {
        if (error) *error = QStringLiteral("Missing hardware profile catalog: ") + directory;
        return {};
    }
    for (const auto &name : path.entryList({QStringLiteral("*.yaml")}, QDir::Files, QDir::Name)) {
        try {
            const YAML::Node root = YAML::LoadFile(path.filePath(name).toStdString());
            if (!root.IsMap()) throw std::runtime_error("Profile must be a mapping");
            HardwareProfile profile;
            const QString profileName = field(root, "id");
            if (!validPackage(profileName)) throw std::runtime_error("Invalid profile ID");
            profile.id = QStringLiteral("support:") + profileName;
            profile.name = field(root, "name");
            profile.chineseName = field(root, "name_zh");
            profile.description = field(root, "description");
            profile.chineseDescription = field(root, "description_zh");
            profile.category = category(field(root, "category"));
            profile.source = ProfileSource::Support;
            profile.packages = list(root["packages"]);
            profile.services = list(root["services"]);
            if (ids.contains(profile.id) || profile.name.isEmpty()
                || profile.category == DeviceCategory::Other
                || (profile.packages.isEmpty() && profile.services.isEmpty())
                || !std::all_of(profile.packages.begin(), profile.packages.end(), validPackage)
                || !std::all_of(profile.services.begin(), profile.services.end(), validService))
                throw std::runtime_error("Invalid profile fields");
            const YAML::Node criteria = root["match"];
            if (!criteria || !criteria.IsMap() || !criteria["any"]
                || !criteria["any"].IsSequence() || criteria["any"].size() == 0)
                throw std::runtime_error("Missing match rules");
            for (const auto &node : criteria["any"])
                profile.matchRules.append(readMatch(node));
            ids.insert(profile.id);
            result.append(std::move(profile));
        } catch (const std::exception &exception) {
            if (error) *error = QStringLiteral("%1: %2").arg(name, QString::fromUtf8(exception.what()));
            return {};
        }
    }
    return result;
}

bool ProfileCatalog::matches(const HardwareProfile &profile, const HardwareDevice &device)
{
    for (const auto &rule : profile.matchRules) {
        if (matchesRule(rule, device)) return true;
    }
    return false;
}
