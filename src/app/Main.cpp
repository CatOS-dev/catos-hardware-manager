#include "HardwareManagerWindow.h"
#include "widgets/HardwareUi.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QIcon>
#include <QLibraryInfo>
#include <QLocale>
#include <QTranslator>

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    app.setOrganizationName(QStringLiteral("CatOS"));
    app.setApplicationName(QStringLiteral("CatOS Hardware Manager"));
    app.setWindowIcon(HardwareUi::catosBrandIcon());
    // Qt's own strings: standard dialog buttons such as Cancel and Close.
    QTranslator qtTranslator;
    if (qtTranslator.load(QLocale(), QStringLiteral("qtbase"), QStringLiteral("_"),
                          QLibraryInfo::path(QLibraryInfo::TranslationsPath)))
        app.installTranslator(&qtTranslator);
    QTranslator translator;
    const auto script = HardwareUi::preferredChinese();
    if (script != HardwareUi::ChineseScript::None) {
        const QString file = script == HardwareUi::ChineseScript::Traditional
            ? QStringLiteral("catos-hardware-manager_zh_TW") : QStringLiteral("catos-hardware-manager_zh_CN");
        const QString bin = QCoreApplication::applicationDirPath();
        for (const auto &dir : {bin, bin + QStringLiteral("/translations"),
                                QDir::cleanPath(bin + QStringLiteral("/../share/catos-hardware-manager/translations"))}) {
            if (translator.load(file, dir)) {
                app.installTranslator(&translator);
                break;
            }
        }
    }
    HardwareManagerWindow window;
    window.show();
    return app.exec();
}
