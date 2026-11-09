#include "HardwareManagerWindow.h"

#include "dialogs/ApplyDialog.h"
#include "pages/DevicesPage.h"
#include "pages/InstalledProfilesPage.h"
#include "pages/ProfileCatalogPage.h"
#include "services/HardwareService.h"
#include "services/HardwareMonitor.h"
#include "services/DiagnosticReport.h"
#include <QCloseEvent>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QPlainTextEdit>
#include <QSaveFile>
#include <QStatusBar>
#include "widgets/HardwareUi.h"

#include <QApplication>
#include <QFrame>
#include <QFutureWatcher>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPalette>
#include <QPushButton>
#include <QStackedWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <QtConcurrentRun>

HardwareManagerWindow::HardwareManagerWindow(QWidget *parent) : QMainWindow(parent)
{
    setWindowTitle(tr("CatOS Hardware Manager"));
    resize(1080, 720);
    setMinimumSize(800, 540);

    auto *root = new QWidget(this);
    auto *horizontal = new QHBoxLayout(root);
    horizontal->setContentsMargins(0, 0, 0, 0);
    horizontal->setSpacing(0);

    auto *sidebar = new QWidget(root);
    sidebar->setFixedWidth(234);
    auto sidebarPalette = sidebar->palette();
    sidebarPalette.setColor(QPalette::Window, palette().color(QPalette::Window));
    sidebar->setPalette(sidebarPalette);
    sidebar->setAutoFillBackground(true);
    auto *left = new QVBoxLayout(sidebar);
    left->setContentsMargins(24, 28, 16, 22);
    left->setSpacing(6);

    auto *branding = new QWidget(sidebar);
    auto *brand = new QHBoxLayout(branding);
    brand->setContentsMargins(6, 0, 0, 12);
    brand->setSpacing(12);
    auto *symbol = new QLabel(branding);
    symbol->setFixedSize(42, 42);
    symbol->setPixmap(HardwareUi::catosBrandIcon().pixmap(QSize(42, 42)));
    brand->addWidget(symbol);
    auto *brandText = new QVBoxLayout;
    brandText->setSpacing(0);
    brandText->addWidget(HardwareUi::heading(QStringLiteral("CatOS"), branding, 4));
    brandText->addWidget(HardwareUi::heading(QStringLiteral("Hardware"), branding, 2));
    brand->addLayout(brandText);
    brand->addStretch();
    left->addWidget(branding);
    left->addSpacing(24);

    const QStringList labels{tr("Hardware devices"), tr("Installed profiles"), tr("Profile catalog")};
    const QStringList icons{QStringLiteral("computer"), QStringLiteral("package"), QStringLiteral("layers")};
    for (int i = 0; i < labels.size(); ++i) {
        auto *button = new QPushButton(sidebar);
        button->setFlat(true);
        button->setCheckable(true);
        button->setMinimumHeight(54);
        button->setCursor(Qt::PointingHandCursor);
        button->setFocusPolicy(Qt::StrongFocus);
        button->setAccessibleName(labels[i]);
        button->setStyleSheet(QStringLiteral(
            "QPushButton { background: transparent; border: 0; border-radius: 11px; }"
            "QPushButton:checked { background: palette(alternate-base); }"
            "QPushButton:hover:!checked { background: palette(alternate-base); }"
            "QPushButton:focus { border: 1px solid palette(highlight); }"));
        auto *line = new QHBoxLayout(button);
        line->setContentsMargins(12, 5, 8, 5);
        line->setSpacing(13);
        line->addWidget(new SymbolLabel(icons[i], 24, button));
        auto *text = new QLabel(labels[i], button);
        line->addWidget(text, 1);
        for (auto *child : button->findChildren<QWidget *>())
            child->setAttribute(Qt::WA_TransparentForMouseEvents);
        navigation_.append(button);
        navLabels_.append(text);
        connect(button, &QPushButton::clicked, this, [this, i] { showPage(i); });
        left->addWidget(button);
        left->addSpacing(3);
    }
    left->addStretch();
    auto *history = new QPushButton(tr("Last operation"), sidebar);
    connect(history, &QPushButton::clicked, this, [this] { ApplyDialog::showLastResult(this); });
    left->addWidget(history);
    horizontal->addWidget(sidebar);

    auto *separator = new QFrame(root);
    separator->setFrameShape(QFrame::VLine);
    separator->setFrameShadow(QFrame::Plain);
    horizontal->addWidget(separator);

    auto *right = new QWidget(root);
    auto *vertical = new QVBoxLayout(right);
    vertical->setContentsMargins(0, 0, 0, 0);
    vertical->setSpacing(0);
    stack_ = new QStackedWidget(right);
    devices_ = new DevicesPage(stack_);
    installed_ = new InstalledProfilesPage(stack_);
    catalog_ = new ProfileCatalogPage(stack_);
    stack_->addWidget(devices_);
    stack_->addWidget(installed_);
    stack_->addWidget(catalog_);
    vertical->addWidget(stack_, 1);

    footer_ = new QFrame(right);
    footer_->setFrameShape(QFrame::StyledPanel);
    auto barPalette = footer_->palette();
    barPalette.setColor(QPalette::Window, barPalette.color(QPalette::AlternateBase));
    footer_->setPalette(barPalette);
    footer_->setAutoFillBackground(true);
    auto *actions = new QHBoxLayout(footer_);
    actions->setContentsMargins(30, 12, 30, 12);
    pending_ = new QLabel(footer_);
    actions->addWidget(pending_, 1);
    discard_ = new QPushButton(tr("Discard changes"), footer_);
    connect(discard_, &QPushButton::clicked, this, [this] {
        selection_.reset(snapshot_);
        showData();
    });
    actions->addWidget(discard_);
    apply_ = new QPushButton(tr("Apply"), footer_);
    connect(apply_, &QPushButton::clicked, this, [this] {
        if (!ready_) return;
        applying_ = true;
        const ApplyResult result = ApplyDialog::execute(this, snapshot_, selection_);
        applying_ = false;
        if (result != ApplyResult::Cancelled)
            refresh(result == ApplyResult::Failed);
    });
    actions->addWidget(apply_);
    vertical->addWidget(footer_);
    footer_->hide();
    horizontal->addWidget(right, 1);
    setCentralWidget(root);

    watcher_ = new QFutureWatcher<HardwareSnapshot>(this);
    connect(watcher_, &QFutureWatcher<HardwareSnapshot>::finished, this, [this] {
        snapshot_ = watcher_->result();
        ready_ = snapshot_.isValid();
        devices_->setRefreshing(false);
        statusBar()->showMessage(ready_ ? tr("Hardware information refreshed") : tr("Hardware query failed"));
        if (!ready_) {
            showData();
            if (refreshQueued_) {
                refreshQueued_ = false;
                QTimer::singleShot(0, this, [this] { refresh(true); });
            }
            return;
        }
        selection_.refresh(snapshot_, refreshRetain_);
        showData();
        if (refreshQueued_) {
            refreshQueued_ = false;
            QTimer::singleShot(0, this, [this] { refresh(true); });
        }
    });

    connect(devices_, &DevicesPage::reportRequested, this, &HardwareManagerWindow::exportReport);
    auto *monitor = new HardwareMonitor(this);
    auto *debounce = new QTimer(this);
    debounce->setSingleShot(true);
    debounce->setInterval(800);
    connect(monitor, &HardwareMonitor::changed, debounce, [debounce] { debounce->start(); });
    connect(debounce, &QTimer::timeout, this, [this, debounce] {
        if (applying_) { debounce->start(); return; }
        refresh(true);
    });
    connect(devices_, &DevicesPage::refreshRequested, this, [this] { refresh(true); });
    connect(devices_, &DevicesPage::recommendedRequested, this, &HardwareManagerWindow::selectRecommended);
    // Defer rebuilding the pages until the originating checkbox mouse event
    // has finished: a direct rebuild destroys the live QCheckBox.
    connect(devices_, &DevicesPage::profileToggled, this,
            &HardwareManagerWindow::selectProfile, Qt::QueuedConnection);
    connect(installed_, &InstalledProfilesPage::profileToggled, this,
            &HardwareManagerWindow::selectProfile, Qt::QueuedConnection);
    connect(catalog_, &ProfileCatalogPage::profileToggled, this,
            &HardwareManagerWindow::selectProfile, Qt::QueuedConnection);
    showPage(0);
    QTimer::singleShot(0, this, [this] { refresh(); });
}

void HardwareManagerWindow::showPage(int index)
{
    stack_->setCurrentIndex(index);
    for (int i = 0; i < navigation_.size(); ++i) {
        navigation_[i]->setChecked(index == i);
        QFont font = navLabels_[i]->font();
        font.setBold(index == i);
        navLabels_[i]->setFont(font);
    }
}

void HardwareManagerWindow::refresh(bool keepSelection)
{
    if (watcher_->isRunning()) { refreshQueued_ = true; return; }
    if (applying_) return;
    // Pages stay usable while refreshing; results are applied in place.
    statusBar()->showMessage(tr("Refreshing hardware information…"));
    devices_->setRefreshing(true);
    ready_ = false;
    refreshRetain_ = keepSelection;
    updateFooter();
    watcher_->setFuture(QtConcurrent::run([] { return HardwareService::read(); }));
}

void HardwareManagerWindow::showData()
{
    devices_->updateState(snapshot_, selection_);
    installed_->updateState(snapshot_, selection_);
    catalog_->updateState(snapshot_, selection_);
    updateFooter();
}

void HardwareManagerWindow::updateFooter()
{
    const int pending = selection_.pendingCount(snapshot_);
    footer_->setVisible(snapshot_.isValid() && pending > 0);
    pending_->setText(tr("%1 pending changes").arg(pending));
    discard_->setEnabled(ready_ && pending > 0);
    apply_->setEnabled(ready_ && pending > 0);
}

void HardwareManagerWindow::selectProfile(const QString &id, bool selected)
{
    // Selection works on the last valid snapshot, also while a refresh runs;
    // a refresh keeps pending choices. Always resync so a rejected toggle
    // puts the checkbox back.
    if (!snapshot_.isValid()) return;
    if (!selection_.select(id, selected, snapshot_))
        statusBar()->showMessage(tr("This profile cannot be changed right now."), 5000);
    showData();
}

void HardwareManagerWindow::selectRecommended()
{
    if (!snapshot_.isValid()) return;
    // Only adds: an installed driver in the same conflict group is kept.
    selection_.selectRecommended(snapshot_);
    showData();
}

void HardwareManagerWindow::closeEvent(QCloseEvent *event)
{
    if (applying_) {
        event->ignore();
        statusBar()->showMessage(tr("Wait for the current operation to finish before closing."));
        return;
    }
    if (selection_.pendingCount(snapshot_) > 0 && QMessageBox::question(this, tr("Discard changes"),
        tr("Close and discard pending changes?")) != QMessageBox::Yes) {
        event->ignore();
        return;
    }
    QMainWindow::closeEvent(event);
}

void HardwareManagerWindow::exportReport()
{
    QDialog dialog(this);
    dialog.setWindowTitle(tr("Diagnostic report preview"));
    dialog.resize(720, 500);
    auto *layout = new QVBoxLayout(&dialog);
    layout->addWidget(HardwareUi::muted(tr("This report excludes serial numbers, device names, addresses, hostnames and raw logs. Review it before saving."), &dialog, true));
    const auto contents = DiagnosticReport::create(snapshot_);
    auto *preview = new QPlainTextEdit(&dialog);
    preview->setReadOnly(true);
    preview->setPlainText(QString::fromUtf8(contents));
    layout->addWidget(preview);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, &dialog);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);
    if (dialog.exec() != QDialog::Accepted) return;
    const auto path = QFileDialog::getSaveFileName(this, tr("Save diagnostic report"), QStringLiteral("hardware-report.json"), tr("JSON files (*.json)"));
    if (path.isEmpty()) return;
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(contents) != contents.size() || !file.commit())
        QMessageBox::warning(this, tr("Hardware Manager"), tr("Could not save diagnostic report."));
}
