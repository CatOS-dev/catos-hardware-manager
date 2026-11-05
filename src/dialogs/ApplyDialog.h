#pragma once

#include "model/HardwareTypes.h"
#include "model/ProfileSelection.h"

#include <QWidget>

enum class ApplyResult { Cancelled, Applied, Failed };

class ApplyDialog {
public:
    static void showLastResult(QWidget *parent);
    // Confirmation is entirely local to the application, never a chat action.
    // Returns Cancelled without changing the pending selection.
    static ApplyResult execute(QWidget *parent, const HardwareSnapshot &snapshot,
                        const ProfileSelection &selection);
};
