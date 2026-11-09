#pragma once

#include "model/HardwareTypes.h"
#include "model/ProfileSelection.h"
#include "services/ApplyPreflight.h"
#include "widgets/Presentation.h"

#include <QDialog>
#include <QWidget>

class QVBoxLayout;
class StatusLabel;

enum class ApplyResult { Cancelled, AuthCancelled, Applied, Failed };

// The confirmation step: what will change, and what the user should know
// before authorising it. Preflight facts arrive asynchronously.
class ApplyConfirmation : public QDialog {
    Q_OBJECT
public:
    ApplyConfirmation(const HardwareSnapshot &snapshot, const ProfileSelection &selection,
                      bool runPreflight, QWidget *parent = nullptr);
    void setPreflight(const ApplyPreflight &preflight);
    QStringList notes() const;
    QStringList groupTitles() const { return groupTitles_; }
private:
    void addNote(const QString &text, StatusLevel level);
    bool changesDrivers_ = false;
    QStringList groupTitles_;
    QVBoxLayout *notes_ = nullptr;
    StatusLabel *checking_ = nullptr;
};

class ApplyDialog {
public:
    static void showLastResult(QWidget *parent);
    // Warn once if the last root transaction log ends without a verdict.
    static void checkInterrupted(QWidget *parent);
    // Returns Cancelled or AuthCancelled without changing anything.
    static ApplyResult execute(QWidget *parent, const HardwareSnapshot &snapshot,
                               const ProfileSelection &selection);
};
