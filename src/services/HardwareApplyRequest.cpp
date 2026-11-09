#include "HardwareApplyRequest.h"

#include <QRegularExpression>

namespace {
bool validName(const QString &value)
{
    static const QRegularExpression profile(QStringLiteral("^[a-z0-9][a-z0-9._-]{0,127}$"));
    return profile.match(value).hasMatch();
}
}

namespace HardwareApplyRequestParser {
bool read(const QStringList &arguments, HardwareApplyRequest &request)
{
    QSet<QString> used;
    for (int i = 1; i < arguments.size(); i += 2) {
        if (i + 1 >= arguments.size()) return false;
        const QString operation = arguments[i];
        const QString name = arguments[i + 1];
        if (operation == QLatin1String("--expect")) {
            if (!validName(name) || used.contains(QStringLiteral("expect:") + name)) return false;
            request.expectedChwd.insert(name);
            used.insert(QStringLiteral("expect:") + name);
            continue;
        }
        if (operation == QLatin1String("--support-install") || operation == QLatin1String("--disable-services")) {
            if (!name.startsWith(QLatin1String("support:"))
                || !validName(name.mid(8)) || used.contains(name)) return false;
            if (operation == QLatin1String("--support-install")) request.support.append(name);
            else request.disableServices.append(name);
        } else if (operation == QLatin1String("--install")
                   || operation == QLatin1String("--remove")) {
            if (!validName(name) || used.contains(name)) return false;
            if (operation == QLatin1String("--install")) request.install.append(name);
            else request.remove.append(name);
        } else return false;
        used.insert(name);
    }
    return !request.remove.isEmpty() || !request.install.isEmpty() || !request.support.isEmpty()
        || !request.disableServices.isEmpty();
}

bool validate(const HardwareApplyRequest &request, const HardwareSnapshot &snapshot)
{
    QSet<QString> current;
    for (const auto &profile : snapshot.profiles)
        if (profile.source == ProfileSource::Chwd && profile.installed) current.insert(profile.chwdName);
    if (current != request.expectedChwd) return false;

    QSet<QString> finalChwd = current;
    for (const auto &name : request.remove) {
        const auto *profile = snapshot.profile(QStringLiteral("chwd:") + name);
        if (!profile || !profile->installed) return false;
        finalChwd.remove(name);
    }
    for (const auto &name : request.install) {
        const auto *profile = snapshot.profile(QStringLiteral("chwd:") + name);
        if (!profile || profile->installed || !profile->applicable) return false;
        finalChwd.insert(name);
    }
    QSet<QString> groups;
    for (const auto &name : finalChwd) {
        const auto *profile = snapshot.profile(QStringLiteral("chwd:") + name);
        if (profile && !profile->conflictGroup.isEmpty() && groups.contains(profile->conflictGroup)) return false;
        if (profile && !profile->conflictGroup.isEmpty()) groups.insert(profile->conflictGroup);
    }
    for (const auto &id : request.support) {
        const auto *profile = snapshot.profile(id);
        if (!profile || profile->source != ProfileSource::Support
            || !profile->applicable || profile->ready || !profile->conflictingPackages.isEmpty()) return false;
        for (const auto &otherId : request.support) {
            if (otherId == id) continue;
            const auto *other = snapshot.profile(otherId);
            if (!other) return false;
            for (const auto &package : profile->conflicts)
                if (other->packages.contains(package)) return false;
            for (const auto &package : other->conflicts)
                if (profile->packages.contains(package)) return false;
        }
    }
    QSet<QString> enabling;
    for (const auto &id : request.support)
        if (const auto *profile = snapshot.profile(id)) enabling.unite(QSet<QString>(profile->services.cbegin(), profile->services.cend()));
    for (const auto &id : request.disableServices) {
        const auto *profile = snapshot.profile(id);
        if (!profile || profile->source != ProfileSource::Support || !profile->ready
            || profile->services.isEmpty()) return false;
        for (const auto &service : profile->services)
            if (enabling.contains(service)) return false;
    }
    return true;
}
}
