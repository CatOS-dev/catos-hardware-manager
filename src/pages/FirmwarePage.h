#pragma once
#include "model/HardwareTypes.h"
#include <QWidget>
#include <QJsonArray>

class QProcess;
class QPlainTextEdit;
class QTreeWidget;
class QPushButton;
class QLabel;

class FirmwarePage : public QWidget {
    Q_OBJECT
public:
    explicit FirmwarePage(QWidget *parent = nullptr, const QString &program = QStringLiteral("/usr/bin/fwupdmgr"));
    void updateState(const HardwareSnapshot &snapshot);
    bool isBusy() const { return busy_; }
signals:
    void refreshRequested();
private:
    void start(const QString &action, const QString &device = {});
    void render();
    QProcess *process_ = nullptr;
    QTreeWidget *devices_ = nullptr;
    QPlainTextEdit *log_ = nullptr;
    QPushButton *check_ = nullptr;
    QPushButton *update_ = nullptr;
    QPushButton *setup_ = nullptr;
    QLabel *status_ = nullptr;
    QJsonArray hardware_;
    QJsonArray updates_;
    QByteArray output_;
    QString action_;
    QString program_;
    bool busy_ = false;
    bool available_ = false;
};
