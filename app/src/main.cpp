#include "Logging.hpp"
#include "dante/ui/AppController.hpp"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

int main(int argc, char* argv[]) {
    QGuiApplication app(argc, argv);
    QGuiApplication::setOrganizationName("DanteCLI");
    QGuiApplication::setApplicationName("DANTE CLI");

    dante::installFileLogger();

    dante::AppController controller;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("App", &controller);
    engine.loadFromModule("Dante", "Main");

    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
