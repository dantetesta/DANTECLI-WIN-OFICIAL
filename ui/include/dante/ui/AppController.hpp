#pragma once

#include <QObject>
#include <QString>

namespace dante {

// Bridge between QML and the app. F0: just static identity strings.
class AppController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString appName READ appName CONSTANT)
    Q_PROPERTY(QString version READ version CONSTANT)

public:
    explicit AppController(QObject* parent = nullptr);

    QString appName() const;
    QString version() const;
};

} // namespace dante
