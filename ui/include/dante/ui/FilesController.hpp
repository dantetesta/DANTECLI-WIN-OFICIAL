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
    Q_INVOKABLE QString parentDir(const QString& path) const;

    // Ações do menu de contexto. As de FS retornam true em sucesso (QML recarrega a árvore).
    Q_INVOKABLE void revealInOS(const QString& path) const;     // Finder/Explorer com o item selecionado
    Q_INVOKABLE void copyPath(const QString& path) const;       // caminho -> clipboard
    Q_INVOKABLE bool makeFolder(const QString& parent, const QString& name) const;
    Q_INVOKABLE bool makeFile(const QString& parent, const QString& name) const;
    Q_INVOKABLE bool renamePath(const QString& path, const QString& newName) const;
    Q_INVOKABLE bool trashPath(const QString& path) const;

    // F5 — abrir arquivos como abas de editor/preview.
    Q_INVOKABLE QString fileKind(const QString& path) const; // editor|image|video|audio|other
    Q_INVOKABLE QVariantMap readFile(const QString& path) const; // {ok, text, plain, tooBig}
    Q_INVOKABLE bool writeFile(const QString& path, const QString& text) const; // salva UTF-8
    Q_INVOKABLE void openExternally(const QString& path) const;  // abre no app padrão do SO

signals:
    void rootChanged();
    void showHiddenChanged();

private:
    QString root_;
    bool showHidden_ = false;
};

} // namespace dante
