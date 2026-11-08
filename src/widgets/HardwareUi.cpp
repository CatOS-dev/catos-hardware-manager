#include "HardwareUi.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QLocale>
#include <QPainter>
#include <QPushButton>
#include <QSvgRenderer>
#include <QVBoxLayout>
#include <QFrame>

namespace {
QString shape(const QString &name)
{
    if (name == QLatin1String("video-display"))
        return QStringLiteral("<rect x='3' y='4' width='18' height='14' rx='2'/><path d='M8 22h8M12 18v4'/>");
    if (name == QLatin1String("network-wireless"))
        return QStringLiteral("<path d='M2 9a16 16 0 0 1 20 0M5 12a11 11 0 0 1 14 0M8 15a6 6 0 0 1 8 0'/>"
                              "<circle cx='12' cy='19' r='1' fill='currentColor' stroke='none'/>");
    if (name == QLatin1String("network-wired"))
        return QStringLiteral("<rect x='4' y='4' width='16' height='11' rx='2'/>"
                              "<path d='M8 15v5h8v-5M8 8h8M12 8v4'/>");
    if (name == QLatin1String("audio-card"))
        return QStringLiteral("<rect x='3' y='7' width='18' height='10' rx='2'/>"
                              "<circle cx='10' cy='12' r='2'/><path d='M17 10v4'/>");
    if (name == QLatin1String("bluetooth"))
        return QStringLiteral("<path d='m7 7 10 10-5 4V3l5 4L7 17'/>");
    if (name == QLatin1String("camera-web"))
        return QStringLiteral("<rect x='3' y='5' width='18' height='14' rx='2'/>"
                              "<circle cx='12' cy='12' r='4'/><circle cx='18' cy='8' r='.8'/>");
    if (name == QLatin1String("printer"))
        return QStringLiteral("<path d='M6 9V3h12v6M6 17H4V9h16v8h-2M6 14h12v7H6z'/>");
    if (name == QLatin1String("scanner"))
        return QStringLiteral("<path d='M5 4h14M4 8h16v11H4zM7 12h10M7 15h10'/>");
    if (name == QLatin1String("input-tablet"))
        return QStringLiteral("<rect x='3' y='4' width='15' height='16' rx='2'/>"
                              "<path d='m13 15 7-9 2 2-7 9-3 1z'/>");
    if (name == QLatin1String("fingerprint"))
        return QStringLiteral("<path d='M8 21c1-3 1-7 1-9a3 3 0 0 1 6 0c0 2 0 5 1 8M5 18c1-2 1-5 1-7a6 6 0 0 1 12 0c0 2 0 5 2 7M11 21c.5-2 1-5 1-8a1 1 0 0 1 2 0c0 3 .5 6 1 8'/>");
    if (name == QLatin1String("virtual-machine"))
        return QStringLiteral("<rect x='3' y='4' width='18' height='13' rx='2'/><path d='M8 21h8M12 17v4M7 9h10M7 12h5'/>");
    if (name == QLatin1String("battery"))
        return QStringLiteral("<rect x='3' y='6' width='17' height='12' rx='2'/><path d='M20 10h2v4h-2M6 9v6h4V9z'/>");
    if (name == QLatin1String("input-gaming"))
        return QStringLiteral("<path d='M7 8h10a6 6 0 0 1 6 6v3a3 3 0 0 1-5 2l-3-3H9l-3 3a3 3 0 0 1-5-2v-3a6 6 0 0 1 6-6zM6 12v5M3.5 14.5h5M17 12h.1M20 15h.1'/>");
    if (name == QLatin1String("smartcard"))
        return QStringLiteral("<rect x='2' y='5' width='20' height='14' rx='2'/>"
                              "<rect x='6' y='9' width='7' height='6' rx='1'/><path d='M16 10h3M16 14h3'/>");
    if (name == QLatin1String("firmware") || name == QLatin1String("computer"))
        return QStringLiteral("<rect x='5' y='5' width='14' height='14' rx='2'/>"
                              "<path d='M9 9h6v6H9zM9 2v3M15 2v3M9 19v3M15 19v3M2 9h3M2 15h3M19 9h3M19 15h3'/>");
    if (name == QLatin1String("package"))
        return QStringLiteral("<path d='m12 2 9 5-9 5-9-5 9-5zM3 7v10l9 5 9-5V7M12 12v10'/>");
    if (name == QLatin1String("layers"))
        return QStringLiteral("<path d='m12 2 9 5-9 5-9-5 9-5zM3 12l9 5 9-5M3 17l9 5 9-5'/>");
    if (name == QLatin1String("refresh"))
        return QStringLiteral("<path d='M20 7v5h-5M4 17v-5h5M5 10a8 8 0 0 1 13-3l2 5M4 12l2 5a8 8 0 0 0 13-3'/>");
    if (name == QLatin1String("chevron-down"))
        return QStringLiteral("<path d='m6 9 6 6 6-6'/>");
    if (name == QLatin1String("chevron-right"))
        return QStringLiteral("<path d='m9 5 7 7-7 7'/>");
    return QStringLiteral("<circle cx='12' cy='12' r='9'/><path d='M12 7v10M7 12h10'/>");
}
}

QIcon HardwareUi::catosBrandIcon()
{
    // Use the original CatOS branding SVG; never draw a substitute logo.
    const QString logo = QStringLiteral("/usr/share/pixmaps/catos-logo.svg");
    if (QFileInfo(logo).isFile()) return QIcon(logo);
    return QIcon::fromTheme(QStringLiteral("catos-logo"));
}

QPixmap HardwareUi::symbol(const QString &name, const QPalette &palette, int size)
{
    const QString ink = palette.color(QPalette::WindowText).name();
    const QString xml = QStringLiteral(
        "<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24'>"
        "<g fill='none' stroke='%1' stroke-width='1.7' stroke-linecap='round'"
        " stroke-linejoin='round'>%2</g></svg>").arg(ink, shape(name));
    QPixmap image(size, size);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    QSvgRenderer(xml.toUtf8()).render(&painter);
    return image;
}

QLabel *HardwareUi::muted(const QString &text, QWidget *parent, bool wrap)
{
    auto *label = new QLabel(text, parent);
    label->setForegroundRole(QPalette::PlaceholderText);
    label->setWordWrap(wrap);
    return label;
}

QLabel *HardwareUi::heading(const QString &text, QWidget *parent, int extraPoints)
{
    auto *label = new QLabel(text, parent);
    auto font = label->font();
    font.setBold(true);
    font.setPointSize(font.pointSize() + extraPoints);
    label->setFont(font);
    return label;
}

void HardwareUi::clear(QLayout *layout)
{
    while (auto *item = layout->takeAt(0)) {
        if (item->widget()) delete item->widget();
        if (item->layout()) clear(item->layout());
        delete item;
    }
}

void HardwareUi::separator(QVBoxLayout *layout, QWidget *parent)
{
    auto *line = new QFrame(parent);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Plain);
    layout->addWidget(line);
}

QWidget *HardwareUi::header(const QString &title, QPushButton *action, QWidget *parent)
{
    auto *widget = new QWidget(parent);
    auto *layout = new QHBoxLayout(widget);
    layout->setContentsMargins(0, 0, 0, 22);
    layout->addWidget(heading(title, widget, 10));
    layout->addStretch();
    if (action) layout->addWidget(action);
    return widget;
}

QString HardwareUi::translatedName(const HardwareProfile &profile)
{
    return QLocale::system().language() == QLocale::Chinese && !profile.chineseName.isEmpty()
        ? profile.chineseName : profile.name;
}

QString HardwareUi::translatedDescription(const HardwareProfile &profile)
{
    return QLocale::system().language() == QLocale::Chinese && !profile.chineseDescription.isEmpty()
        ? profile.chineseDescription : profile.description;
}
