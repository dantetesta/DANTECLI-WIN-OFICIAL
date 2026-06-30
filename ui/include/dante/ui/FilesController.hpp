#pragma once

#include <QObject>
#include <QString>
#include <QUrl>
#include <QVariantList>

namespace dante {

// Navegação real de pastas para a sidebar. Lista um nível por vez (árvore lazy no QML).
class FilesController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString root READ root WRITE setRoot NOTIFY rootChanged)
    Q_PROPERTY(bool showHidden READ showHidden WRITE setShowHidden NOTIFY showHiddenChanged)

public:
    explicit FilesController(QObject* parent = nullptr);

    QString root() const { return root_; }
    void setRoot(const QString& p);
    bool showHidden() const { return showHidden_; }
    void setShowHidden(bool v);

    // [{name, dir, path}], pastas primeiro, respeitando showHidden. Vazio = lista o root.
    Q_INVOKABLE QVariantList list(const QString& path = QString()) const;
    Q_INVOKABLE QString home() const;
    Q_INVOKABLE QString standard(const QString& which) const; // home|desktop|documents|downloads
    Q_INVOKABLE QString baseName(const QString& path) const;   // último componente (label)
    Q_INVOKABLE QString displayPath(const QString& path) const; // home -> "~"
    Q_INVOKABLE QString fromUrl(const QUrl& u) const { return u.toLocalFile(); }
    Q_INVOKABLE void toggleHidden() { setShowHidden(!showHidden_); }

signals:
    void rootChanged();
    void showHiddenChanged();

private:
    QString root_;
    bool showHidden_ = false;
};

} // namespace dante
