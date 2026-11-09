#include "Cli.h"
#include "HardwareManagerWindow.h"
#include "widgets/HardwareUi.h"

#include <QApplication>

int main(int argc, char **argv)
{
    // Headless modes must not need a display.
    if (Cli::wants(argc, argv)) return Cli::run(argc, argv);

    QApplication app(argc, argv);
    app.setOrganizationName(QStringLiteral("CatOS"));
    app.setApplicationName(QStringLiteral("CatOS Hardware Manager"));
    app.setWindowIcon(HardwareUi::catosBrandIcon());
    HardwareUi::installTranslations();
    HardwareManagerWindow window;
    window.show();
    return app.exec();
}
