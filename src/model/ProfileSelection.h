#pragma once

#include "HardwareTypes.h"

class ProfileSelection {
public:
    void refresh(const HardwareSnapshot &state, bool retainPending);
    bool select(const QString &id, bool enabled, const HardwareSnapshot &state);
    void selectRecommended(const HardwareSnapshot &state);
    void reset(const HardwareSnapshot &state);
    bool selected(const QString &id) const { return selected_.contains(id); }
    QStringList installingChwd(const HardwareSnapshot &state) const;
    QStringList removingChwd(const HardwareSnapshot &state) const;
    QStringList enablingSupport(const HardwareSnapshot &state) const;
    // Stopping a ready support profile's services is an explicit action,
    // separate from the checkbox, so unchecking can never mean "disable".
    static bool canDisableServices(const HardwareProfile &profile);
    bool stageServiceDisable(const QString &id, bool staged, const HardwareSnapshot &state);
    bool serviceDisableStaged(const QString &id) const { return disableServices_.contains(id); }
    QStringList disablingServices(const HardwareSnapshot &state) const;
    int pendingCount(const HardwareSnapshot &state) const;
    const QSet<QString> &baseline() const { return baseline_; }
private:
    QSet<QString> selected_;
    QSet<QString> baseline_;
    QSet<QString> readySupport_;
    QSet<QString> disableServices_;
};
