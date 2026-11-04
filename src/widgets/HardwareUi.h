#pragma once

#include "model/HardwareTypes.h"

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
QPixmap symbol(const QString &name, const QPalette &palette, int size = 24);
QLabel *muted(const QString &text, QWidget *parent, bool wrap = false);
QLabel *heading(const QString &text, QWidget *parent, int extraPoints = 0);
void clear(QLayout *layout);
void separator(QVBoxLayout *layout, QWidget *parent);
QWidget *header(const QString &title, QPushButton *action, QWidget *parent);
QString translatedName(const HardwareProfile &profile);
QString translatedDescription(const HardwareProfile &profile);
}
