#include "Logging.hpp"
#include "dante/ui/AppController.hpp"
#include "dante/ui/TerminalController.hpp"

#include <QGuiApplication>
#include <QImage>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QTimer>

#include <string_view>

int main(int argc, char* argv[]) {
    QGuiApplication app(argc, argv);
    QGuiApplication::setOrganizationName("DanteCLI");
    QGuiApplication::setApplicationName("DANTE CLI");

    // --selftest: smoke headless p/ o CI rodar o .exe empacotado no runner Windows.
    // Sobe Qt, carrega o QML, salva um print e sai com 0 (ok) / 3 (QML falhou). Os
    // resultados vão pro dante.log (app é GUI, sem stderr/console).
    bool selftest = false;
    for (int i = 1; i < argc; ++i) {
        if (std::string_view(argv[i]) == "--selftest") {
            selftest = true;
        }
    }

    dante::installFileLogger();

    dante::AppController controller;
    dante::TerminalController terminal;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("App", &controller);
    engine.rootContext()->setContextProperty("Term", &terminal);
    engine.loadFromModule("Dante", "Main");

    const bool qmlOk = !engine.rootObjects().isEmpty();

    if (selftest) {
        qWarning("SELFTEST qml=%s objs=%d", qmlOk ? "OK" : "FAIL",
                 static_cast<int>(engine.rootObjects().size()));
        if (!qmlOk) {
            return 3;
        }
        auto* win = qobject_cast<QQuickWindow*>(engine.rootObjects().constFirst());
        if (win == nullptr) {
            qWarning("SELFTEST window=none");
            return 0;
        }
        win->setVisible(true);
        QTimer::singleShot(1200, &app, [&app, win] {
            const QImage img = win->grabWindow();
            const QString path = QCoreApplication::applicationDirPath() + "/dante-selftest.png";
            const bool saved = !img.isNull() && img.save(path);
            qWarning("SELFTEST screenshot=%dx%d saved=%d", img.width(), img.height(),
                     saved ? 1 : 0);
            app.exit(0);
        });
        return app.exec();
    }

    if (!qmlOk) {
        return -1;
    }
    return app.exec();
}
