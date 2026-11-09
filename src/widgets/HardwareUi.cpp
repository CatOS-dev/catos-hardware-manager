#include "HardwareUi.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QLocale>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QSvgRenderer>
#include <QVBoxLayout>
#include <QFrame>
#include <QApplication>
#include <QEvent>
#include <QStyle>
#include <QtMath>

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
    if (name == QLatin1String("cpu"))
        return QStringLiteral("<rect x='4' y='4' width='16' height='16' rx='2'/><path d='M8 1v3M12 1v3M16 1v3M8 20v3M12 20v3M16 20v3M1 8h3M1 12h3M1 16h3M20 8h3M20 12h3M20 16h3'/>");
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
    if (name == QLatin1String("status-ok"))
        return QStringLiteral("<circle cx='12' cy='12' r='9'/><path d='m8 12.5 3 3 5-6'/>");
    if (name == QLatin1String("status-attention"))
        return QStringLiteral("<path d='M12 3 2 20h20zM12 10v4M12 17h.01'/>");
    if (name == QLatin1String("status-error"))
        return QStringLiteral("<circle cx='12' cy='12' r='9'/><path d='m9 9 6 6M15 9l-6 6'/>");
    if (name == QLatin1String("status-pending"))
        return QStringLiteral("<circle cx='12' cy='12' r='9'/><path d='M12 7v5l3 2'/>");
    if (name == QLatin1String("status-neutral"))
        return QStringLiteral("<circle cx='12' cy='12' r='3'/>");
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

QPixmap HardwareUi::symbol(const QString &name, const QColor &color, int size, qreal devicePixelRatio)
{
    const QString xml = QStringLiteral(
        "<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24'>"
        "<g fill='none' stroke='%1' stroke-width='1.7' stroke-linecap='round'"
        " stroke-linejoin='round'>%2</g></svg>").arg(color.name(), shape(name));
    const qreal ratio = qMax<qreal>(1.0, devicePixelRatio);
    QPixmap image(qCeil(size * ratio), qCeil(size * ratio));
    image.fill(Qt::transparent);
    QPainter painter(&image);
    QSvgRenderer(xml.toUtf8()).render(&painter);
    painter.end();
    image.setDevicePixelRatio(ratio);
    return image;
}

QPixmap HardwareUi::symbol(const QString &name, const QPalette &palette, int size)
{
    return symbol(name, palette.color(QPalette::WindowText), size, qApp ? qApp->devicePixelRatio() : 1.0);
}

namespace {
qreal luminance(const QColor &color)
{
    const auto channel = [](qreal value) {
        return value <= 0.03928 ? value / 12.92 : qPow((value + 0.055) / 1.055, 2.4);
    };
    return 0.2126 * channel(color.redF()) + 0.7152 * channel(color.greenF()) + 0.0722 * channel(color.blueF());
}

qreal contrast(const QColor &a, const QColor &b)
{
    const qreal x = luminance(a), y = luminance(b);
    return (qMax(x, y) + 0.05) / (qMin(x, y) + 0.05);
}

QColor mix(const QColor &a, const QColor &b, qreal amount)
{
    return QColor::fromRgbF(a.redF() + (b.redF() - a.redF()) * amount,
                            a.greenF() + (b.greenF() - a.greenF()) * amount,
                            a.blueF() + (b.blueF() - a.blueF()) * amount);
}
}

QColor HardwareUi::statusColor(StatusLevel level, const QPalette &palette)
{
    const QColor background = palette.color(QPalette::Window);
    const QColor text = palette.color(QPalette::WindowText);
    const bool dark = luminance(background) < 0.25;
    QColor color;
    switch (level) {
    case StatusLevel::Ok: color = QColor(dark ? 0x8fd694 : 0x1e6b25); break;
    case StatusLevel::Pending: color = QColor(dark ? 0x9ecbff : 0x1a5fb4); break;
    case StatusLevel::Attention: color = QColor(dark ? 0xffc266 : 0x8a4b00); break;
    case StatusLevel::Error: color = QColor(dark ? 0xffaba3 : 0xb3261e); break;
    case StatusLevel::Neutral: color = mix(text, background, 0.3); break;
    }
    // Custom palettes can defeat the defaults: move toward the text colour
    // until the minimum contrast holds.
    for (int step = 0; step < 10 && contrast(color, background) < 4.5; ++step)
        color = mix(color, text, 0.2);
    return color;
}

const QVector<DeviceCategory> &HardwareUi::categoryOrder()
{
    static const QVector<DeviceCategory> order{
        DeviceCategory::Graphics, DeviceCategory::Network, DeviceCategory::Audio,
        DeviceCategory::Bluetooth, DeviceCategory::Camera, DeviceCategory::Printer,
        DeviceCategory::Scanner, DeviceCategory::Tablet, DeviceCategory::Fingerprint,
        DeviceCategory::Virtualization, DeviceCategory::Power, DeviceCategory::Cpu,
        DeviceCategory::Thunderbolt, DeviceCategory::Controller, DeviceCategory::MobileBroadband,
        DeviceCategory::SmartCard, DeviceCategory::Other
    };
    return order;
}

QString HardwareUi::focusedRowKey()
{
    for (QWidget *focus = QApplication::focusWidget(); focus; focus = focus->parentWidget()) {
        const QString key = focus->property("rowKey").toString();
        if (!key.isEmpty()) return key;
    }
    return {};
}

QString HardwareUi::statusSymbol(StatusLevel level)
{
    switch (level) {
    case StatusLevel::Ok: return QStringLiteral("status-ok");
    case StatusLevel::Pending: return QStringLiteral("status-pending");
    case StatusLevel::Attention: return QStringLiteral("status-attention");
    case StatusLevel::Error: return QStringLiteral("status-error");
    case StatusLevel::Neutral: return QStringLiteral("status-neutral");
    }
    return {};
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

SymbolLabel::SymbolLabel(const QString &name, int size, QWidget *parent)
    : QLabel(parent), name_(name), size_(size)
{
    setFixedSize(size, size);
    setAccessibleName(QString());
    render();
}

void SymbolLabel::setSymbol(const QString &name)
{
    if (name_ == name) return;
    name_ = name;
    render();
}

void SymbolLabel::setLevel(StatusLevel level)
{
    coloured_ = true;
    level_ = level;
    render();
}

void SymbolLabel::changeEvent(QEvent *event)
{
    QLabel::changeEvent(event);
    if (event->type() == QEvent::PaletteChange || event->type() == QEvent::StyleChange
        || event->type() == QEvent::DevicePixelRatioChange)
        render();
}

void SymbolLabel::render()
{
    const QColor color = coloured_ ? HardwareUi::statusColor(level_, palette())
                                   : palette().color(QPalette::WindowText);
    setPixmap(HardwareUi::symbol(name_, color, size_, devicePixelRatioF()));
}

StatusLabel::StatusLabel(QWidget *parent) : QWidget(parent)
{
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);
    icon_ = new SymbolLabel(QStringLiteral("status-neutral"), 16, this);
    layout->addWidget(icon_, 0, Qt::AlignTop);
    text_ = new QLabel(this);
    text_->setWordWrap(true);
    layout->addWidget(text_, 1);
    setStatus({});
}

QString StatusLabel::text() const
{
    return text_->text();
}

void StatusLabel::setStatus(const StatusText &status)
{
    status_ = status;
    text_->setText(status.text);
    icon_->setSymbol(HardwareUi::statusSymbol(status.level));
    icon_->setLevel(status.level);
    setVisible(!status.text.isEmpty());
    setAccessibleName(status.text);
    applyColour();
}

void StatusLabel::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::PaletteChange) applyColour();
}

void StatusLabel::applyColour()
{
    QPalette p = text_->palette();
    p.setColor(QPalette::WindowText, HardwareUi::statusColor(status_.level, palette()));
    text_->setPalette(p);
}

NoticeBanner::NoticeBanner(QWidget *parent) : QFrame(parent)
{
    setAutoFillBackground(true);
    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(14, 10, 14, 10);
    outer->setSpacing(6);
    auto *line = new QHBoxLayout;
    line->setSpacing(10);
    icon_ = new SymbolLabel(QStringLiteral("status-attention"), 20, this);
    line->addWidget(icon_, 0, Qt::AlignTop);
    summary_ = new QLabel(this);
    summary_->setWordWrap(true);
    summary_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    line->addWidget(summary_, 1);
    toggle_ = new QPushButton(tr("Show all"), this);
    toggle_->setCheckable(true);
    toggle_->setFlat(true);
    line->addWidget(toggle_, 0, Qt::AlignTop);
    outer->addLayout(line);
    details_ = new QWidget(this);
    auto *detailsLayout = new QVBoxLayout(details_);
    detailsLayout->setContentsMargins(30, 0, 0, 0);
    list_ = new QLabel(details_);
    list_->setWordWrap(true);
    list_->setTextFormat(Qt::PlainText);
    list_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    detailsLayout->addWidget(list_);
    outer->addWidget(details_);
    details_->hide();
    connect(toggle_, &QPushButton::toggled, this, [this](bool shown) {
        details_->setVisible(shown);
        toggle_->setText(shown ? tr("Hide") : tr("Show all"));
        toggle_->setAccessibleDescription(shown ? tr("Notice details shown") : tr("Notice details hidden"));
    });
    toggle_->setAccessibleDescription(tr("Notice details hidden"));
    hide();
}

void NoticeBanner::setNotices(const QString &error, const QStringList &warnings)
{
    notices_ = warnings;
    if (!error.isEmpty()) notices_.prepend(tr("Hardware query failed: %1").arg(error));
    level_ = error.isEmpty() ? StatusLevel::Attention : StatusLevel::Error;
    setVisible(!notices_.isEmpty());
    if (notices_.isEmpty()) return;
    summary_->setText(notices_.size() == 1 ? notices_.first()
                      : tr("%1 (and %n more notice(s))", nullptr, int(notices_.size() - 1)).arg(notices_.first()));
    toggle_->setVisible(notices_.size() > 1);
    QStringList rest = notices_.mid(1);
    for (auto &item : rest) item.prepend(QStringLiteral("• "));
    list_->setText(rest.join(QLatin1Char('\n')));
    icon_->setSymbol(HardwareUi::statusSymbol(level_));
    setAccessibleName(summary_->text());
    applyColours();
}

void NoticeBanner::changeEvent(QEvent *event)
{
    QFrame::changeEvent(event);
    // setPalette() in applyColours() raises another PaletteChange.
    if (event->type() == QEvent::PaletteChange && !applying_) applyColours();
}

void NoticeBanner::applyColours()
{
    // Tint the background lightly and keep the text at full contrast.
    const QPalette base = parentWidget() ? parentWidget()->palette() : palette();
    const QColor accent = HardwareUi::statusColor(level_, base);
    QPalette p = palette();
    QColor tint = accent;
    tint.setAlphaF(1.0);
    const QColor window = base.color(QPalette::Window);
    p.setColor(QPalette::Window, QColor::fromRgbF(window.redF() * 0.88 + tint.redF() * 0.12,
                                                  window.greenF() * 0.88 + tint.greenF() * 0.12,
                                                  window.blueF() * 0.88 + tint.blueF() * 0.12));
    p.setColor(QPalette::WindowText, base.color(QPalette::WindowText));
    applying_ = true;
    setPalette(p);
    applying_ = false;
    icon_->setLevel(level_);
}

RowButton::RowButton(QWidget *parent) : QAbstractButton(parent)
{
    setCheckable(true);
    setFocusPolicy(Qt::StrongFocus);
    setCursor(Qt::PointingHandCursor);
    setAttribute(Qt::WA_Hover);
}

void RowButton::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    const QRectF area = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    QPainterPath path;
    path.addRoundedRect(area, 10, 10);
    if (underMouse() || isDown() || (highlightChecked_ && isChecked()))
        painter.fillPath(path, palette().color(QPalette::AlternateBase));
    if (hasFocus()) {
        painter.setPen(QPen(palette().color(QPalette::Highlight), 1.5));
        painter.drawPath(path);
    }
}
