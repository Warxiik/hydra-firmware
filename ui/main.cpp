#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QFont>
#include "backend/bridge.h"
#include "backend/printer_model.h"
#include "backend/file_model.h"

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    app.setApplicationName("Hydra");
    app.setOrganizationName("HydraPrinter");
    app.setApplicationVersion(HYDRA_VERSION_STRING);

    /* Dark theme with Material style */
    QQuickStyle::setStyle("Material");
    qputenv("QT_QUICK_CONTROLS_MATERIAL_THEME", "Dark");
    qputenv("QT_QUICK_CONTROLS_MATERIAL_ACCENT", "#00BCD4"); /* Teal accent */

    /* Default font */
    QFont defaultFont("Inter", 14);
    app.setFont(defaultFont);

    /* Register QML types */
    qmlRegisterType<hydra::ui::PrinterBridge>("Hydra.Backend", 1, 0, "PrinterBridge");
    qmlRegisterType<hydra::ui::FileModel>("Hydra.Backend", 1, 0, "FileModel");

    QQmlApplicationEngine engine;
    engine.load(QUrl(QStringLiteral("qrc:/Hydra/qml/Main.qml")));

    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    return app.exec();
}
