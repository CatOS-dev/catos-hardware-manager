#pragma once

#include "model/HardwareTypes.h"
#include "model/ProfileSelection.h"

#include <QMainWindow>
#include <QVector>

class QFutureWatcherBase;
class QLabel;
class QPushButton;
class RowButton;
class QStackedWidget;
class QFrame;
class DevicesPage;
class InstalledProfilesPage;
class ProfileCatalogPage;
class QCloseEvent;
template <typename T> class QFutureWatcher;

class HardwareManagerWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit HardwareManagerWindow(QWidget *parent = nullptr);

protected:
    void closeEvent(QCloseEvent *event) override;
private:
    void exportReport();
    void refresh(bool keepSelection = false);
    void showPage(int index);
    void showData();
    void updateFooter();
    void selectProfile(const QString &id, bool selected);
    void selectRecommended();
    void stageServiceStop(const QString &id, bool staged);

    HardwareSnapshot snapshot_;
    ProfileSelection selection_;
    bool ready_ = false;
    bool applying_ = false;
    bool refreshQueued_ = false;
    bool refreshRetain_ = false;
    QFutureWatcher<HardwareSnapshot> *watcher_ = nullptr;
    QVector<RowButton *> navigation_;
    QVector<QLabel *> navLabels_;
    QStackedWidget *stack_ = nullptr;
    DevicesPage *devices_ = nullptr;
    InstalledProfilesPage *installed_ = nullptr;
    ProfileCatalogPage *catalog_ = nullptr;
    QFrame *footer_ = nullptr;
    QLabel *pending_ = nullptr;
    QPushButton *discard_ = nullptr;
    QPushButton *apply_ = nullptr;
};
