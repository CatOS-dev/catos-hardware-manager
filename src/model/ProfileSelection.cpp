#include "ProfileSelection.h"

#include <algorithm>

namespace {
QStringList sorted(QStringList items)
{
    items.sort(Qt::CaseInsensitive);
    return items;
}
}

void ProfileSelection::reset(const HardwareSnapshot &state)
{
    selected_.clear();
    for (const auto &profile : state.profiles) {
        if (profile.source == ProfileSource::Chwd ? profile.installed : profile.ready)
            selected_.insert(profile.id);
    }
}

void ProfileSelection::refresh(const HardwareSnapshot &state, bool retainPending)
{
    if (!retainPending) {
        reset(state);
    } else {
        QSet<QString> known;
        for (const auto &profile : state.profiles) known.insert(profile.id);
        selected_.intersect(known);
        for (const auto &id : readySupport_) selected_.remove(id);
        for (const auto &profile : state.profiles) {
            if (profile.source == ProfileSource::Support && profile.ready)
                selected_.insert(profile.id);
            // An externally installed CHWD profile was never staged for removal.
            if (profile.source == ProfileSource::Chwd && profile.installed
                && !baseline_.contains(profile.chwdName))
                selected_.insert(profile.id);
        }
    }
    readySupport_.clear();
    for (const auto &profile : state.profiles)
        if (profile.source == ProfileSource::Support && profile.ready) readySupport_.insert(profile.id);
    baseline_.clear();
    for (const auto &profile : state.profiles) {
        if (profile.source == ProfileSource::Chwd && profile.installed)
            baseline_.insert(profile.chwdName);
    }
}

bool ProfileSelection::select(const QString &id, bool enabled, const HardwareSnapshot &state)
{
    const auto *profile = state.profile(id);
    if (!profile || (!profile->applicable && (profile->source == ProfileSource::Support || !profile->installed))) return false;
    if (profile->systemManaged && profile->ready && !enabled) return false;
    if (enabled && !profile->conflictingPackages.isEmpty()) return false;
    if (enabled) {
        if (profile->source == ProfileSource::Support) {
            for (const auto &other : state.profiles) {
                if (other.id == id || !selected_.contains(other.id)
                    || other.source != ProfileSource::Support)
                    continue;
                for (const auto &package : profile->conflicts)
                    if (other.packages.contains(package)) return false;
                for (const auto &package : other.conflicts)
                    if (profile->packages.contains(package)) return false;
            }
        }
        if (profile->source == ProfileSource::Chwd && !profile->conflictGroup.isEmpty()) {
            for (const auto &other : state.profiles) {
                if (other.id != id && other.source == ProfileSource::Chwd
                    && other.conflictGroup == profile->conflictGroup)
                    selected_.remove(other.id);
            }
        }
        selected_.insert(id);
    } else {
        selected_.remove(id);
    }
    return true;
}

void ProfileSelection::selectRecommended(const HardwareSnapshot &state)
{
    for (const auto &device : state.devices) {
        const auto *profile = state.profile(device.recommendedProfile);
        if (!profile || profile->source != ProfileSource::Chwd) continue;
        if (!profile->conflictGroup.isEmpty()) {
            bool installedInGroup = false;
            for (const auto &other : state.profiles) {
                if (other.source == ProfileSource::Chwd && other.installed
                    && other.conflictGroup == profile->conflictGroup) {
                    installedInGroup = true;
                    break;
                }
            }
            if (installedInGroup) continue;
        }
        select(profile->id, true, state);
    }
    for (const auto &profile : state.profiles) {
        if (profile.source == ProfileSource::Support && profile.recommended
            && profile.applicable && !profile.ready && profile.conflictingPackages.isEmpty())
            select(profile.id, true, state);
    }
}

QStringList ProfileSelection::installingChwd(const HardwareSnapshot &state) const
{
    QStringList names;
    for (const auto &profile : state.profiles) {
        if (profile.source == ProfileSource::Chwd && selected_.contains(profile.id)
            && !profile.installed) names.append(profile.chwdName);
    }
    return sorted(names);
}

QStringList ProfileSelection::removingChwd(const HardwareSnapshot &state) const
{
    QStringList names;
    for (const auto &profile : state.profiles) {
        if (profile.source == ProfileSource::Chwd && !selected_.contains(profile.id)
            && profile.installed) names.append(profile.chwdName);
    }
    return sorted(names);
}

QStringList ProfileSelection::enablingSupport(const HardwareSnapshot &state) const
{
    QStringList names;
    for (const auto &profile : state.profiles) {
        if (profile.source == ProfileSource::Support && profile.applicable && selected_.contains(profile.id)
            && !profile.ready) names.append(profile.id);
    }
    return sorted(names);
}

int ProfileSelection::pendingCount(const HardwareSnapshot &state) const
{
    return installingChwd(state).size() + removingChwd(state).size() + enablingSupport(state).size();
}
