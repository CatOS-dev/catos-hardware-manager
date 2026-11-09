#include "ApplyDialog.h"

#include "TransactionProtocol.h"
#include "widgets/HardwareUi.h"

#include <QApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QDateTime>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QLabel>
#include <QLocale>
#include <QMap>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QProcess>
#include <QProgressBar>
#include <QPushButton>
#include <QSaveFile>
#include <QScrollArea>
#include <QSettings>
#include <QStandardPaths>
#include <QToolButton>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QtConcurrentRun>

namespace {
const QString kRootLog = QStringLiteral("/var/log/catos-hardware-manager/last-transaction.log");

QString resultPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)
        + QStringLiteral("/last-transaction.log");
}

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

void showText(QWidget *parent, const QString &title, const QString &text)
{
    QDialog dialog(parent);
    dialog.setWindowTitle(title);
    dialog.resize(740, 500);
    auto *layout = new QVBoxLayout(&dialog);
    auto *view = new QPlainTextEdit(&dialog);
    view->setReadOnly(true);
    view->setPlainText(text);
    layout->addWidget(view);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, &dialog);
    auto *copy = buttons->addButton(QObject::tr("Copy log"), QDialogButtonBox::ActionRole);
    QObject::connect(copy, &QPushButton::clicked, &dialog, [view] {
        QApplication::clipboard()->setText(view->toPlainText());
    });
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);
    dialog.exec();
}
}

ApplyConfirmation::ApplyConfirmation(const HardwareSnapshot &snapshot, const ProfileSelection &selection,
                                     bool runPreflight, QWidget *parent)
    : QDialog(parent)
{
    const QStringList toRemove = selection.removingChwd(snapshot);
    const QStringList toInstall = selection.installingChwd(snapshot);
    const QStringList support = selection.enablingSupport(snapshot);
    const QStringList stopServices = selection.disablingServices(snapshot);
    changesDrivers_ = !toRemove.isEmpty() || !toInstall.isEmpty();

    setWindowTitle(tr("Apply hardware configurations"));
    resize(620, 460);
    auto *layout = new QVBoxLayout(this);
    auto *scroller = new QScrollArea(this);
    scroller->setFrameShape(QFrame::NoFrame);
    scroller->setWidgetResizable(true);
    auto *container = new QWidget(scroller);
    auto *details = new QVBoxLayout(container);
    details->setContentsMargins(8, 9, 8, 12);
    details->setSpacing(6);

    const auto group = [&](const QString &title, const QStringList &ids, bool chwd, StatusLevel level) {
        if (ids.isEmpty()) return;
        groupTitles_.append(title);
        auto *heading = new StatusLabel(container);
        heading->setStatus({title, level});
        QFont font = heading->font();
        font.setBold(true);
        heading->setFont(font);
        details->addWidget(heading);
        for (const auto &id : ids) {
            const auto *profile = snapshot.profile(chwd ? QStringLiteral("chwd:") + id : id);
            QString text = profile ? HardwareUi::translatedName(*profile) : id;
            if (profile && !chwd && level == StatusLevel::Attention && !profile->services.isEmpty())
                text += QStringLiteral(" (") + profile->services.join(QStringLiteral(", ")) + QLatin1Char(')');
            auto *label = new QLabel(QStringLiteral("    ") + text, container);
            label->setWordWrap(true);
            details->addWidget(label);
        }
        details->addSpacing(10);
    };
    // Removing a driver is the destructive part; list it first.
    group(tr("Remove drivers"), toRemove, true, StatusLevel::Error);
    group(tr("Install drivers"), toInstall, true, StatusLevel::Pending);
    group(tr("Install hardware support"), support, false, StatusLevel::Pending);
    group(tr("Stop services"), stopServices, false, StatusLevel::Attention);

    auto *toggle = new QToolButton(container);
    toggle->setText(tr("Package and service changes"));
    toggle->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    toggle->setArrowType(Qt::RightArrow);
    toggle->setCheckable(true);
    toggle->setAutoRaise(true);
    toggle->setAccessibleDescription(tr("Collapsed"));
    details->addWidget(toggle, 0, Qt::AlignLeft);
    auto *packages = new QWidget(container);
    auto *packageLayout = new QVBoxLayout(packages);
    packageLayout->setContentsMargins(18, 0, 0, 0);
    const auto describe = [&](const HardwareProfile &profile, const QStringList &items) {
        packageLayout->addWidget(HardwareUi::heading(HardwareUi::translatedName(profile), packages));
        if (!items.isEmpty())
            packageLayout->addWidget(HardwareUi::muted(items.join(QStringLiteral(", ")), packages, true));
    };
    for (const auto &name : toRemove + toInstall)
        if (const auto *profile = snapshot.profile(QStringLiteral("chwd:") + name)) describe(*profile, profile->packages);
    for (const auto &id : support)
        if (const auto *profile = snapshot.profile(id)) describe(*profile, profile->missingPackages + profile->services);
    for (const auto &id : stopServices)
        if (const auto *profile = snapshot.profile(id)) describe(*profile, profile->services);
    packages->hide();
    details->addWidget(packages);
    connect(toggle, &QToolButton::toggled, this, [toggle, packages](bool expanded) {
        packages->setVisible(expanded);
        toggle->setArrowType(expanded ? Qt::DownArrow : Qt::RightArrow);
        toggle->setAccessibleDescription(expanded ? tr("Expanded") : tr("Collapsed"));
    });
    details->addStretch();
    scroller->setWidget(container);
    layout->addWidget(scroller, 1);

    notes_ = new QVBoxLayout;
    notes_->setSpacing(6);
    layout->addLayout(notes_);
    if (changesDrivers_)
        addNote(tr("Restart the computer after the driver change."), StatusLevel::Attention);
    if (!toRemove.isEmpty() && toInstall.isEmpty())
        addNote(tr("Without a replacement, the device falls back to the generic kernel driver."), StatusLevel::Attention);
    if (!stopServices.isEmpty())
        addNote(tr("Stopped services stay disabled after a restart. Their packages stay installed."), StatusLevel::Neutral);
    addNote(tr("Changes are applied in order. Completed steps may remain if a later step fails."), StatusLevel::Neutral);
    if (runPreflight) {
        checking_ = new StatusLabel(this);
        checking_->setStatus({tr("Checking packages and system state…"), StatusLevel::Neutral});
        notes_->addWidget(checking_);
        auto *watcher = new QFutureWatcher<ApplyPreflight>(this);
        connect(watcher, &QFutureWatcher<ApplyPreflight>::finished, this, [this, watcher] {
            setPreflight(watcher->result());
        });
        watcher->setFuture(QtConcurrent::run([snapshot, toRemove, toInstall, support] {
            return Preflight::run(snapshot, toRemove, toInstall, support);
        }));
    }

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Cancel, this);
    auto *accept = buttons->addButton(tr("Confirm and apply"), QDialogButtonBox::AcceptRole);
    connect(accept, &QPushButton::clicked, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    // A destructive confirmation should not be the Enter default.
    buttons->button(QDialogButtonBox::Cancel)->setDefault(!toRemove.isEmpty());
    accept->setDefault(toRemove.isEmpty());
    layout->addWidget(buttons);
}

void ApplyConfirmation::addNote(const QString &text, StatusLevel level)
{
    auto *note = new StatusLabel(this);
    note->setStatus({text, level});
    notes_->addWidget(note);
}

void ApplyConfirmation::setPreflight(const ApplyPreflight &preflight)
{
    if (checking_) {
        checking_->deleteLater();
        notes_->removeWidget(checking_);
        checking_ = nullptr;
    }
    if (!preflight.kernelsWithoutHeaders.isEmpty())
        addNote(tr("The DKMS driver cannot be built for kernels without headers: %1. Install the matching headers package first.")
                    .arg(preflight.kernelsWithoutHeaders.join(QStringLiteral(", "))), StatusLevel::Error);
    if (preflight.secureBoot)
        addNote(tr("Secure Boot is enabled. Modules built by DKMS must be signed before the kernel loads them."),
                StatusLevel::Attention);
    if (preflight.updatesPending)
        addNote(tr("System updates are available. Update the whole system first to avoid a partial upgrade during the driver change."),
                StatusLevel::Attention);
    if (changesDrivers_)
        addNote(preflight.snapshots ? tr("A snapper snapshot is created before the drivers change.")
                                    : tr("snapper is not configured; no snapshot will be created."),
                preflight.snapshots ? StatusLevel::Ok : StatusLevel::Neutral);
    if (preflight.downloadBytes > 0)
        addNote(tr("Download size: %1").arg(QLocale().formattedDataSize(preflight.downloadBytes)), StatusLevel::Neutral);
}

QStringList ApplyConfirmation::notes() const
{
    QStringList texts;
    for (int i = 0; i < notes_->count(); ++i)
        if (auto *label = qobject_cast<StatusLabel *>(notes_->itemAt(i)->widget())) texts.append(label->text());
    return texts;
}

ApplyResult ApplyDialog::execute(QWidget *parent, const HardwareSnapshot &snapshot,
                                 const ProfileSelection &selection)
{
    const QStringList toRemove = selection.removingChwd(snapshot);
    const QStringList toInstall = selection.installingChwd(snapshot);
    const QStringList support = selection.enablingSupport(snapshot);
    const QStringList stopServices = selection.disablingServices(snapshot);
    if (toRemove.isEmpty() && toInstall.isEmpty() && support.isEmpty() && stopServices.isEmpty())
        return ApplyResult::Cancelled;

    ApplyConfirmation confirm(snapshot, selection, true, parent);
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
    for (const auto &id : stopServices) arguments << QStringLiteral("--disable-services") << id;

    RunningDialog progress(parent);
    progress.setWindowTitle(QObject::tr("Applying hardware configurations"));
    progress.resize(660, 420);
    auto *progressLayout = new QVBoxLayout(&progress);
    auto *message = new QLabel(QObject::tr("Waiting for administrator authorization…"), &progress);
    message->setWordWrap(true);
    progressLayout->addWidget(message);
    auto *bar = new QProgressBar(&progress);
    bar->setRange(0, 0);
    progressLayout->addWidget(bar);
    auto *steps = new QTreeWidget(&progress);
    steps->setHeaderLabels({QObject::tr("Operation"), QObject::tr("Result")});
    steps->setRootIsDecorated(false);
    steps->setColumnWidth(0, 440);
    progressLayout->addWidget(steps, 1);
    auto *logToggle = new QToolButton(&progress);
    logToggle->setText(QObject::tr("Command output"));
    logToggle->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    logToggle->setArrowType(Qt::RightArrow);
    logToggle->setCheckable(true);
    logToggle->setAutoRaise(true);
    progressLayout->addWidget(logToggle, 0, Qt::AlignLeft);
    auto *log = new QPlainTextEdit(&progress);
    log->setReadOnly(true);
    log->setMaximumBlockCount(5000);
    log->hide();
    progressLayout->addWidget(log, 1);
    QObject::connect(logToggle, &QToolButton::toggled, &progress, [logToggle, log](bool shown) {
        log->setVisible(shown);
        logToggle->setArrowType(shown ? Qt::DownArrow : Qt::RightArrow);
    });
    auto *buttons = new QDialogButtonBox(&progress);
    auto *copy = buttons->addButton(QObject::tr("Copy log"), QDialogButtonBox::ActionRole);
    auto *close = buttons->addButton(QDialogButtonBox::Close);
    close->setEnabled(false);
    QObject::connect(copy, &QPushButton::clicked, &progress, [log] {
        QApplication::clipboard()->setText(log->toPlainText());
    });
    QObject::connect(close, &QPushButton::clicked, &progress, &QDialog::accept);
    progressLayout->addWidget(buttons);

    auto *process = new QProcess(&progress);
    process->setProcessChannelMode(QProcess::MergedChannels);
    QByteArray pending;
    bool sawPlan = false;
    QString failure;
    QMap<int, QTreeWidgetItem *> items;
    auto consume = [&] {
        int pos;
        while ((pos = pending.indexOf('\n')) != -1) {
            const QString line = QString::fromUtf8(pending.left(pos)).trimmed();
            pending.remove(0, pos + 1);
            if (line.isEmpty()) continue;
            log->appendPlainText(line);
            const auto fields = line.split(QLatin1Char('|'));
            if (fields.size() == 5 && fields[0] == QLatin1String("PLAN")) {
                sawPlan = true;
                const QString target = TransactionProtocol::targetName(fields[3], fields[4], snapshot);
                auto *item = new QTreeWidgetItem(steps, {TransactionProtocol::actionName(fields[3])
                    + (target.isEmpty() ? QString() : QStringLiteral(": ") + target), QObject::tr("Waiting")});
                item->setToolTip(0, item->text(0));
                items.insert(fields[1].toInt(), item);
                bar->setRange(0, fields[2].toInt());
                bar->setValue(0);
            } else if (fields.size() == 5 && fields[0] == QLatin1String("STEP")) {
                message->setText(QObject::tr("Step %1 of %2: %3").arg(fields[1], fields[2],
                    TransactionProtocol::actionName(fields[3])));
                if (auto *item = items.value(fields[1].toInt())) item->setText(1, QObject::tr("Running"));
            } else if (fields.size() == 3 && fields[0] == QLatin1String("RESULT")) {
                if (auto *item = items.value(fields[1].toInt()))
                    item->setText(1, fields[2] == QLatin1String("completed") ? QObject::tr("Completed") : QObject::tr("Failed"));
                bar->setValue(fields[1].toInt());
            } else if (fields.size() == 3 && fields[0] == QLatin1String("RECOVERY")) {
                auto *item = new QTreeWidgetItem(steps, {QObject::tr("Restore driver: %1")
                    .arg(TransactionProtocol::targetName(QStringLiteral("install"), fields[1], snapshot)),
                    fields[2] == QLatin1String("completed") ? QObject::tr("Completed") : QObject::tr("Needs attention")});
                Q_UNUSED(item);
            } else if (fields[0] == QLatin1String("ERROR")) {
                const QString text = TransactionProtocol::errorMessage(fields, snapshot);
                if (!text.isEmpty() && failure.isEmpty()) failure = text;
            }
        }
    };
    QObject::connect(process, &QProcess::readyReadStandardOutput, &progress, [&] {
        pending += process->readAllStandardOutput();
        consume();
    });
    ApplyResult result = ApplyResult::Failed;
    auto finish = [&](int code, bool normalExit) {
        pending += process->readAllStandardOutput();
        if (!pending.isEmpty() && !pending.endsWith('\n')) pending += '\n';
        consume();
        for (auto *item : items) {
            if (item->text(1) == QObject::tr("Running")) item->setText(1, QObject::tr("Interrupted"));
        }
        bar->setRange(0, 1);
        bar->setValue(1);
        progress.allowClose();
        close->setEnabled(true);
        close->setDefault(true);
        if (normalExit && TransactionProtocol::authorizationCancelled(code, sawPlan)) {
            // Nothing ran: keep the previous log and the pending changes.
            result = ApplyResult::AuthCancelled;
            message->setText(QObject::tr("Authorization was cancelled. Nothing was changed."));
            return;
        }
        const bool ok = normalExit && code == 0;
        result = ok ? ApplyResult::Applied : ApplyResult::Failed;
        if (ok) {
            message->setText(toRemove.isEmpty() && toInstall.isEmpty()
                ? QObject::tr("Hardware configurations applied.")
                : QObject::tr("Hardware configurations applied. Restart the computer to use the new drivers."));
        } else {
            QString text = failure.isEmpty() ? QObject::tr("Some changes could not be applied.") : failure;
            if (sawPlan) text += QLatin1Char(' ') + QObject::tr("Completed steps were kept. If a snapshot was created, you can restore it with snapper.");
            message->setText(text);
            logToggle->setChecked(true);
        }
        QDir().mkpath(QFileInfo(resultPath()).absolutePath());
        QSaveFile file(resultPath());
        const QByteArray contents = (QDateTime::currentDateTime().toString(Qt::ISODate)
            + QLatin1Char('\n') + message->text() + QLatin1Char('\n') + log->toPlainText()).toUtf8();
        if (!file.open(QIODevice::WriteOnly) || file.write(contents) != contents.size() || !file.commit())
            log->appendPlainText(QObject::tr("Could not save the operation log. Copy it before closing."));
    };
    QObject::connect(process, &QProcess::finished, &progress, [&](int code, QProcess::ExitStatus state) {
        finish(code, state == QProcess::NormalExit);
    });
    QObject::connect(process, &QProcess::errorOccurred, &progress, [&](QProcess::ProcessError code) {
        if (code == QProcess::FailedToStart) {
            log->appendPlainText(process->errorString());
            finish(-1, false);
        }
    });
    process->start(QStringLiteral("/usr/bin/pkexec"), arguments);
    progress.exec();
    return result;
}

void ApplyDialog::showLastResult(QWidget *parent)
{
    QFile file(resultPath());
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::information(parent, QObject::tr("Last operation"), QObject::tr("No saved operation log."));
        return;
    }
    showText(parent, QObject::tr("Last operation"), QString::fromUtf8(file.readAll()));
}

void ApplyDialog::checkInterrupted(QWidget *parent)
{
    QFile file(kRootLog);
    if (!file.open(QIODevice::ReadOnly)) return;
    const QByteArray contents = file.readAll();
    if (!TransactionProtocol::interrupted(contents)) return;
    // Ask once per interrupted transaction, not on every start.
    const QString stamp = QFileInfo(file).lastModified().toString(Qt::ISODateWithMs);
    QSettings settings;
    if (settings.value(QStringLiteral("acknowledgedInterruptedLog")).toString() == stamp) return;
    QMessageBox box(QMessageBox::Warning, QObject::tr("Previous change did not finish"),
        QObject::tr("The last hardware change stopped before it finished, for example because the computer "
                    "was turned off. Drivers may be half installed. Review the log, then refresh and check "
                    "the affected devices."), QMessageBox::Close, parent);
    auto *view = box.addButton(QObject::tr("View log"), QMessageBox::ActionRole);
    box.exec();
    settings.setValue(QStringLiteral("acknowledgedInterruptedLog"), stamp);
    if (box.clickedButton() == view)
        showText(parent, QObject::tr("Interrupted change"), QString::fromUtf8(contents));
}
