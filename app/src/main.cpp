#include "Logging.hpp"
#include "dante/ui/AppController.hpp"
#include "dante/ui/TerminalController.hpp"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

int main(int argc, char* argv[]) {
    QGuiApplication app(argc, argv);
    QGuiApplication::setOrganizationName("DanteCLI");
    QGuiApplication::setApplicationName("DANTE CLI");

    dante::installFileLogger();

    dante::AppController controller;
    dante::TerminalController terminal;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("App", &controller);
    engine.rootContext()->setContextProperty("Term", &terminal);
    engine.loadFromModule("Dante", "Main");

    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
