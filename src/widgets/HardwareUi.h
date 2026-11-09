#pragma once

#include "model/HardwareTypes.h"
#include "Presentation.h"

#include <QAbstractButton>
#include <QFrame>
#include <QLabel>

#include <QIcon>
#include <QPalette>
#include <QPixmap>

class QLabel;
class QLayout;
class QPushButton;
class QVBoxLayout;
class QWidget;

namespace HardwareUi {
QIcon catosBrandIcon();
// Renders at the device pixel ratio so icons stay sharp on HiDPI screens.
QPixmap symbol(const QString &name, const QColor &color, int size, qreal devicePixelRatio);
QPixmap symbol(const QString &name, const QPalette &palette, int size = 24);
// Foreground colour for a status level, adjusted until it reaches a 4.5:1
// contrast ratio against the window background (WCAG AA).
QColor statusColor(StatusLevel level, const QPalette &palette);
QString statusSymbol(StatusLevel level);
// Display order of categories on every page.
const QVector<DeviceCategory> &categoryOrder();
// Keyboard-focused row (property "rowKey") so a rebuild can restore it.
QString focusedRowKey();
QLabel *muted(const QString &text, QWidget *parent, bool wrap = false);
QLabel *heading(const QString &text, QWidget *parent, int extraPoints = 0);
void clear(QLayout *layout);
void separator(QVBoxLayout *layout, QWidget *parent);
QWidget *header(const QString &title, QPushButton *action, QWidget *parent);
// Which Chinese variant the user prefers, judged from the UI language list
// in order (LANGUAGE first), so "en:zh_CN" stays English.
enum class ChineseScript { None, Simplified, Traditional };
ChineseScript chineseScriptFor(const QStringList &uiLanguages);
ChineseScript preferredChinese();
// Install Qt's and the application's translations for the preferred language.
void installTranslations();
QString translatedName(const HardwareProfile &profile);
QString translatedDescription(const HardwareProfile &profile);
}

// An icon label that redraws itself when the palette or the screen's pixel
// ratio changes, so it follows light/dark theme switches.
class SymbolLabel : public QLabel {
    Q_OBJECT
public:
    SymbolLabel(const QString &name, int size, QWidget *parent = nullptr);
    void setSymbol(const QString &name);
    // Draw in a status colour instead of the text colour.
    void setLevel(StatusLevel level);
protected:
    void changeEvent(QEvent *event) override;
private:
    void render();
    QString name_;
    int size_;
    bool coloured_ = false;
    StatusLevel level_ = StatusLevel::Neutral;
};

// Status text with an icon, so the state never relies on colour alone.
class StatusLabel : public QWidget {
    Q_OBJECT
public:
    explicit StatusLabel(QWidget *parent = nullptr);
    void setStatus(const StatusText &status);
    StatusText status() const { return status_; }
    QString text() const;
protected:
    void changeEvent(QEvent *event) override;
private:
    void applyColour();
    SymbolLabel *icon_ = nullptr;
    QLabel *text_ = nullptr;
    StatusText status_;
};

// Page-level notices: one summary line, details on demand.
class NoticeBanner : public QFrame {
    Q_OBJECT
public:
    explicit NoticeBanner(QWidget *parent = nullptr);
    void setNotices(const QString &error, const QStringList &warnings);
    QStringList notices() const { return notices_; }
protected:
    void changeEvent(QEvent *event) override;
private:
    void applyColours();
    SymbolLabel *icon_ = nullptr;
    QLabel *summary_ = nullptr;
    QPushButton *toggle_ = nullptr;
    QWidget *details_ = nullptr;
    QLabel *list_ = nullptr;
    QStringList notices_;
    StatusLevel level_ = StatusLevel::Attention;
    bool applying_ = false;
};

// A flat, checkable row header that draws hover and focus feedback from
// the current palette. Unlike a style-sheeted QPushButton, its children
// keep following palette changes (light/dark switches).
class RowButton : public QAbstractButton {
    Q_OBJECT
public:
    explicit RowButton(QWidget *parent = nullptr);
    // Navigation buttons show their checked state; disclosure headers don't.
    void setHighlightChecked(bool highlight) { highlightChecked_ = highlight; update(); }
protected:
    void paintEvent(QPaintEvent *event) override;
private:
    bool highlightChecked_ = false;
};
