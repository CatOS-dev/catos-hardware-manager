#include "ApplyDialog.h"

#include "widgets/HardwareUi.h"

#include <QCloseEvent>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QProcess>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollArea>
#include <QToolButton>
#include <QVBoxLayout>

namespace {
class RunningDialog : public QDialog {
public:
    using QDialog::QDialog;
    void allowClose() { finished_ = true; }
    void reject() override { if (finished_) QDialog::reject(); }
protected:
    void closeEvent(QCloseEvent *event) override
    {
        if (!finished_) event->ignore();
        else QDialog::closeEvent(event);
    }
private:
    bool finished_ = false;
};

void addGroup(QVBoxLayout *layout, QWidget *parent, const QString &title,
              const QStringList &names, const HardwareSnapshot &snapshot, bool support)
{
    if (names.isEmpty()) return;
    layout->addWidget(HardwareUi::heading(title, parent, 2));
    for (const auto &name : names) {
        const auto *profile = snapshot.profile(support ? name : QStringLiteral("chwd:") + name);
        const QString display = profile ? HardwareUi::translatedName(*profile) : name;
        layout->addWidget(new QLabel(display, parent));
    }
    layout->addSpacing(12);
}
}

ApplyResult ApplyDialog::execute(QWidget *parent, const HardwareSnapshot &snapshot,
                          const ProfileSelection &selection)
{
    const QStringList toRemove = selection.removingChwd(snapshot);
    const QStringList toInstall = selection.installingChwd(snapshot);
    const QStringList support = selection.enablingSupport(snapshot);
    if (toRemove.isEmpty() && toInstall.isEmpty() && support.isEmpty()) return ApplyResult::Cancelled;

    QDialog confirm(parent);
    confirm.setWindowTitle(QObject::tr("Apply hardware configurations"));
    confirm.resize(600, 390);
    auto *layout = new QVBoxLayout(&confirm);
    auto *scroller = new QScrollArea(&confirm);
    scroller->setFrameShape(QFrame::NoFrame);
    scroller->setWidgetResizable(true);
    auto *container = new QWidget(scroller);
    auto *details = new QVBoxLayout(container);
    details->setContentsMargins(8, 9, 8, 12);
    details->setSpacing(9);
    addGroup(details, container, QObject::tr("Remove"), toRemove, snapshot, false);
    addGroup(details, container, QObject::tr("Install"), toInstall, snapshot, false);
    addGroup(details, container, QObject::tr("Hardware support"), support, snapshot, true);
    auto *toggle = new QToolButton(container);
    toggle->setText(QObject::tr("Package and service changes"));
    toggle->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    toggle->setArrowType(Qt::RightArrow);
    toggle->setCheckable(true);
    toggle->setAutoRaise(true);
    details->addWidget(toggle, 0, Qt::AlignLeft);
    auto *packages = new QWidget(container);
    auto *packageLayout = new QVBoxLayout(packages);
    packageLayout->setContentsMargins(0, 0, 0, 0);
    for (const auto &id : support) {
        const auto *profile = snapshot.profile(id);
        if (!profile) continue;
        packageLayout->addWidget(HardwareUi::heading(HardwareUi::translatedName(*profile), packages));
        if (!profile->packages.isEmpty())
            packageLayout->addWidget(HardwareUi::muted(profile->packages.join(QStringLiteral(", ")), packages, true));
        if (!profile->services.isEmpty())
            packageLayout->addWidget(HardwareUi::muted(profile->services.join(QStringLiteral(", ")), packages, true));
    }
    for (const auto &name : toInstall + toRemove) {
        const auto *profile = snapshot.profile(QStringLiteral("chwd:") + name);
        if (!profile) continue;
        packageLayout->addWidget(HardwareUi::heading(HardwareUi::translatedName(*profile), packages));
        packageLayout->addWidget(HardwareUi::muted(profile->packages.join(QStringLiteral(", ")), packages, true));
    }
    packages->hide();
    details->addWidget(packages);
    QObject::connect(toggle, &QToolButton::toggled, &confirm, [toggle, packages](bool expanded) {
        packages->setVisible(expanded);
        toggle->setArrowType(expanded ? Qt::DownArrow : Qt::RightArrow);
    });
    details->addStretch();
    scroller->setWidget(container);
    layout->addWidget(scroller, 1);
    layout->addWidget(HardwareUi::muted(QObject::tr(
        "Changes are applied in order. Completed steps may remain if a later operation fails."),
        &confirm, true));
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Cancel, &confirm);
    auto *accept = buttons->addButton(QObject::tr("Confirm and apply"), QDialogButtonBox::AcceptRole);
    QObject::connect(accept, &QPushButton::clicked, &confirm, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &confirm, &QDialog::reject);
    layout->addWidget(buttons);
    if (confirm.exec() != QDialog::Accepted) return ApplyResult::Cancelled;

    const QString executor = QStringLiteral("/usr/lib/catos/catos-hardware-apply");
    if (!QFileInfo(executor).isExecutable()) {
        QMessageBox::warning(parent, QObject::tr("Hardware Manager"),
                             QObject::tr("The privileged executor is not installed."));
        return ApplyResult::Failed;
    }
    QStringList arguments{executor};
    for (const auto &name : selection.baseline().values())
        arguments << QStringLiteral("--expect") << name;
    for (const auto &name : toRemove) arguments << QStringLiteral("--remove") << name;
    for (const auto &name : toInstall) arguments << QStringLiteral("--install") << name;
    for (const auto &name : support) arguments << QStringLiteral("--support-install") << name;

    RunningDialog progress(parent);
    progress.setWindowTitle(QObject::tr("Applying hardware configurations"));
    progress.resize(620, 300);
    auto *progressLayout = new QVBoxLayout(&progress);
    auto *message = new QLabel(QObject::tr("Requesting administrator authorization…"), &progress);
    progressLayout->addWidget(message);
    auto *bar = new QProgressBar(&progress);
    bar->setRange(0, 0);
    progressLayout->addWidget(bar);
    auto *log = new QPlainTextEdit(&progress);
    log->setReadOnly(true);
    log->hide();
    progressLayout->addWidget(log, 1);
    auto *close = new QPushButton(QObject::tr("Close"), &progress);
    close->setEnabled(false);
    QObject::connect(close, &QPushButton::clicked, &progress, &QDialog::accept);
    progressLayout->addWidget(close, 0, Qt::AlignRight);

    auto *process = new QProcess(&progress);
    process->setProcessChannelMode(QProcess::MergedChannels);
    QByteArray pending;
    bool successful = false;
    QObject::connect(process, &QProcess::readyReadStandardOutput, &progress,
            [process, &pending, message, log, &snapshot] {
        pending += process->readAllStandardOutput();
        int pos;
        while ((pos = pending.indexOf('\n')) != -1) {
            const QString line = QString::fromUtf8(pending.left(pos)).trimmed();
            pending.remove(0, pos + 1);
            if (line.startsWith(QLatin1String("STEP|"))) {
                const auto fields = line.split(QLatin1Char('|'));
                if (fields.size() == 5) {
                    QString display = fields[4];
                    const auto *profile = snapshot.profile(
                        display.startsWith(QLatin1String("support:"))
                            ? display : QStringLiteral("chwd:") + display);
                    if (profile) display = HardwareUi::translatedName(*profile);
                    message->setText(QObject::tr("Applying %1 of %2: %3")
                                     .arg(fields[1], fields[2], display));
                }
            } else if (!line.isEmpty() && line != QLatin1String("DONE")) log->appendPlainText(line);
        }
    });
    auto finish = [&progress, bar, message, log, close, &successful](bool ok) {
        successful = ok;
        bar->setRange(0, 1);
        bar->setValue(1);
        message->setText(ok ? QObject::tr("Hardware configurations applied.")
                            : QObject::tr("Some changes could not be applied."));
        if (!ok) log->show();
        progress.allowClose();
        close->setEnabled(true);
    };
    QObject::connect(process, &QProcess::finished, &progress,
            [&finish](int code, QProcess::ExitStatus state) {
        finish(code == 0 && state == QProcess::NormalExit);
    });
    QObject::connect(process, &QProcess::errorOccurred, &progress,
            [process, log, &finish](QProcess::ProcessError code) {
        if (code == QProcess::FailedToStart) {
            log->appendPlainText(process->errorString());
            finish(false);
        }
    });
    process->start(QStringLiteral("/usr/bin/pkexec"), arguments);
    progress.exec();
    return successful ? ApplyResult::Applied : ApplyResult::Failed;
}
