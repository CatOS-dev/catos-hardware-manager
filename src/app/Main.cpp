#include "HardwareManagerWindow.h"
#include "widgets/HardwareUi.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QIcon>
#include <QLocale>
#include <QTranslator>

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("CatOS Hardware Manager"));
    app.setWindowIcon(HardwareUi::catosBrandIcon());
    QTranslator translator;
    if (QLocale::system().language() == QLocale::Chinese) {
        const QString bin = QCoreApplication::applicationDirPath();
        const QStringList directories{
            bin,
            bin + QStringLiteral("/translations"),
            QDir::cleanPath(bin + QStringLiteral("/../share/catos-hardware-manager/translations"))
        };
        for (const auto &dir : directories) {
            if (translator.load(QStringLiteral("catos-hardware-manager_zh_CN"), dir)) {
                app.installTranslator(&translator);
                break;
            }
        }
    }
    HardwareManagerWindow window;
    window.show();
    return app.exec();
}
