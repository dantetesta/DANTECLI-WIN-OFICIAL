#include "Logging.hpp"
#include "dante/ui/AppController.hpp"
#include "dante/ui/FilesController.hpp"
#include "dante/ui/SysStatsController.hpp"
#include "dante/ui/TerminalController.hpp"

#include <QFile>
#include <QGuiApplication>
#include <QImage>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlError>
#include <QQuickWindow>
#include <QTextStream>
#include <QTimer>
#include <QUrl>
#include <QtQml>

#include <string_view>

int main(int argc, char* argv[]) {
    QGuiApplication app(argc, argv);
    QGuiApplication::setOrganizationName("DanteCLI");
    QGuiApplication::setApplicationName("DANTE CLI");

    bool selftest = false;
    for (int i = 1; i < argc; ++i) {
        if (std::string_view(argv[i]) == "--selftest") {
            selftest = true;
        }
    }

    dante::installFileLogger();

    dante::AppController controller;
    dante::SysStatsController sysStats;
    dante::FilesController files;

    // TerminalController instanciável no QML (grid de terminais: 1 por célula).
    qmlRegisterType<dante::TerminalController>("Backend", 1, 0, "TerminalController");

    QQmlApplicationEngine engine;
    // App empacotado: o engine precisa achar os módulos QML deployados (windeployqt)
    // ao lado do exe. Sem isso, "import QtQuick.Controls.Basic" falha e o QML não carrega
    // -> rootObjects vazio -> a janela nunca aparece.
    engine.addImportPath(QCoreApplication::applicationDirPath() + "/qml");

    engine.rootContext()->setContextProperty("App", &controller);
    engine.rootContext()->setContextProperty("Sys", &sysStats);
    engine.rootContext()->setContextProperty("Files", &files);

    QStringList qmlErrors;
    QObject::connect(&engine, &QQmlApplicationEngine::warnings,
                     [&qmlErrors](const QList<QQmlError>& ws) {
                         for (const auto& w : ws) {
                             qmlErrors << w.toString();
                         }
                     });

    // Carrega o Main.qml direto do recurso embutido. loadFromModule("Dante","Main") falha
    // no Release/MSVC ("Module Dante contains no type named Main" — o registro do tipo some
    // no link), mas o arquivo está no qrc; carregá-lo direto independe do registro por tipo.
    engine.load(QUrl(QStringLiteral("qrc:/Dante/qml/Main.qml")));
    const bool qmlOk = !engine.rootObjects().isEmpty();

    if (selftest) {
        // Relatório robusto num arquivo ao lado do exe (o log às vezes não flusha a tempo).
        QFile f(QCoreApplication::applicationDirPath() + "/selftest.txt");
        if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&f);
            out << "qml=" << (qmlOk ? "OK" : "FAIL") << " objs=" << engine.rootObjects().size()
                << "\n";
            out << "appDir=" << QCoreApplication::applicationDirPath() << "\n";
            out << "importPaths:\n";
            for (const auto& p : engine.importPathList()) {
                out << "  " << p << "\n";
            }
            out << "qmlErrors:\n";
            for (const auto& e : qmlErrors) {
                out << "  " << e << "\n";
            }
        }
        if (!qmlOk) {
            return 3;
        }
        auto* win = qobject_cast<QQuickWindow*>(engine.rootObjects().constFirst());
        if (win == nullptr) {
            return 0;
        }
        win->setVisible(true);
        QTimer::singleShot(1200, &app, [&app, win] {
            const QImage img = win->grabWindow();
            if (!img.isNull()) {
                img.save(QCoreApplication::applicationDirPath() + "/dante-selftest.png");
            }
            app.exit(0);
        });
        return app.exec();
    }

    if (!qmlOk) {
        return -1;
    }
    return app.exec();
}
