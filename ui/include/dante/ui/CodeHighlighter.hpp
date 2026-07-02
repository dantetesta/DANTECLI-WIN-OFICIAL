#pragma once

#include <QObject>
#include <QQuickTextDocument>
#include <QString>

#include <memory>

namespace dante {

// Realce de sintaxe mínimo p/ o editor (F5), sem dependência externa (KSyntaxHighlighting/Monaco
// ficaram fora — ver decisão). Attacha um QSyntaxHighlighter interno ao QTextDocument do TextArea
// (QML: `document: textArea.textDocument`). Regras por linguagem: keywords, strings, comentários,
// números. `enabled=false` (plain-mode em arquivos grandes) desliga o realce.
class CodeHighlighter : public QObject {
    Q_OBJECT
    Q_PROPERTY(QQuickTextDocument* document READ document WRITE setDocument NOTIFY documentChanged)
    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY languageChanged)
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)

public:
    explicit CodeHighlighter(QObject* parent = nullptr);
    ~CodeHighlighter() override;

    QQuickTextDocument* document() const { return doc_; }
    void setDocument(QQuickTextDocument* d);

    QString language() const { return lang_; }
    void setLanguage(const QString& lang);

    bool enabled() const { return enabled_; }
    void setEnabled(bool v);

signals:
    void documentChanged();
    void languageChanged();
    void enabledChanged();

private:
    void rebuild(); // recria o highlighter interno com as regras da linguagem atual

    class Impl; // QSyntaxHighlighter concreto (definido no .cpp)
    QQuickTextDocument* doc_ = nullptr;
    std::unique_ptr<Impl> hl_;
    QString lang_;
    bool enabled_ = true;
};

} // namespace dante
