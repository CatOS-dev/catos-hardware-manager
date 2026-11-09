#pragma once

#include <QDialog>
#include <QVector>

class QLabel;
class QPushButton;
class QTreeWidget;

// A Thunderbolt/USB4 peripheral as reported by boltd.
struct BoltDevice {
    QString path;
    QString uid;
    QString name;
    QString vendor;
    QString type;
    QString status;
    bool stored = false;
};

// Lists and authorises peripherals through boltd's D-Bus API
// (org.freedesktop.bolt). boltd asks polkit for authorisation itself.
class ThunderboltDialog : public QDialog {
    Q_OBJECT
public:
    explicit ThunderboltDialog(QWidget *parent = nullptr, bool connectToBoltd = true);
    // Exposed for tests; normally filled from boltd.
    void setDevices(const QVector<BoltDevice> &devices, const QString &error = {});
    static QString statusText(const BoltDevice &device);
    QTreeWidget *list() const { return list_; }
    QPushButton *authorizeButton() const { return authorize_; }
    QPushButton *forgetButton() const { return forget_; }
private:
    void reload();
    void updateButtons();
    void call(const QString &path, const QString &interface, const QString &method, const QVariantList &arguments);
    const BoltDevice *current() const;
    QVector<BoltDevice> devices_;
    QLabel *message_ = nullptr;
    QTreeWidget *list_ = nullptr;
    QPushButton *authorize_ = nullptr;
    QPushButton *forget_ = nullptr;
    QPushButton *refresh_ = nullptr;
    bool busy_ = false;
};
