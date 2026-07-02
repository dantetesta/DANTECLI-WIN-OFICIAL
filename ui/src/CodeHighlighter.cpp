#include "dante/ui/CodeHighlighter.hpp"

#include <QQuickTextDocument>
#include <QRegularExpression>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QTextDocument>

#include <vector>

namespace dante {

namespace {

// Paleta VSCode-dark.
QColor kKeyword(0x56, 0x9C, 0xD6);
QColor kString(0xCE, 0x91, 0x78);
QColor kComment(0x6A, 0x99, 0x55);
QColor kNumber(0xB5, 0xCE, 0xA8);

// Conjuntos de keywords por família de linguagem. Mínimo — cobre o essencial (PRD: sem dep).
QStringList keywordsFor(const QString& lang) {
    if (lang == "python") {
        return {"def",   "class", "import", "from",  "return", "if",     "elif",  "else",
                "for",   "while", "in",     "not",   "and",    "or",     "try",   "except",
                "with",  "as",    "lambda", "None",  "True",   "False",  "pass",  "break",
                "continue", "yield", "async", "await", "global", "self"};
    }
    if (lang == "shell") {
        return {"if", "then", "else", "elif", "fi", "for", "in", "do", "done", "while",
                "case", "esac", "function", "return", "export", "local", "echo", "cd"};
    }
    if (lang == "json") {
        return {"true", "false", "null"};
    }
    // Família C-like (c/cpp/js/ts/java/cs/go/rust/php…). Superconjunto pragmático.
    return {"const", "let",     "var",    "function", "return", "if",     "else",   "for",
            "while",  "do",      "switch", "case",     "break",  "continue", "new",  "delete",
            "class",  "struct",  "enum",   "public",   "private", "protected", "void", "int",
            "bool",   "float",   "double", "char",     "auto",   "true",   "false",  "null",
            "nullptr", "this",   "import", "export",   "from",   "async",  "await",  "try",
            "catch",  "throw",   "typedef", "namespace", "using", "template", "fn",   "pub",
            "let",    "match",   "impl",   "func",     "type",   "package", "def"};
}

} // namespace

// QSyntaxHighlighter concreto: aplica as regras bloco a bloco + comentário de bloco C-style.
class CodeHighlighter::Impl : public QSyntaxHighlighter {
public:
    // parent = nullptr (NÃO o doc): senão o QTextDocument, dono do QML, também "possuiria" o
    // highlighter e o unique_ptr abaixo faria double-free. setDocument() liga sem transferir posse.
    Impl(QTextDocument* doc, const QString& lang) : QSyntaxHighlighter(static_cast<QObject*>(nullptr)) {
        setDocument(doc);
        QTextCharFormat kwFmt;
        kwFmt.setForeground(kKeyword);
        for (const QString& kw : keywordsFor(lang)) {
            rules_.push_back({QRegularExpression("\\b" + kw + "\\b"), kwFmt});
        }
        QTextCharFormat numFmt;
        numFmt.setForeground(kNumber);
        rules_.push_back({QRegularExpression("\\b[0-9]+(\\.[0-9]+)?\\b"), numFmt});

        strFmt_.setForeground(kString);
        rules_.push_back({QRegularExpression("\"([^\"\\\\]|\\\\.)*\""), strFmt_});
        rules_.push_back({QRegularExpression("'([^'\\\\]|\\\\.)*'"), strFmt_});

        commentFmt_.setForeground(kComment);
        const bool hashComment = (lang == "python" || lang == "shell" || lang == "yaml");
        rules_.push_back({QRegularExpression(hashComment ? "#[^\n]*" : "//[^\n]*"), commentFmt_});

        // Comentário de bloco /* ... */ só p/ C-like.
        blockComments_ = !hashComment && lang != "json";
    }

protected:
    void highlightBlock(const QString& text) override {
        for (const auto& r : rules_) {
            auto it = r.re.globalMatch(text);
            while (it.hasNext()) {
                const auto m = it.next();
                setFormat(static_cast<int>(m.capturedStart()), static_cast<int>(m.capturedLength()),
                          r.fmt);
            }
        }
        if (!blockComments_) {
            return;
        }
        // Multi-linha /* */: estado 1 = dentro de comentário.
        setCurrentBlockState(0);
        static const QRegularExpression startRe("/\\*");
        static const QRegularExpression endRe("\\*/");
        int start = (previousBlockState() == 1) ? 0 : static_cast<int>(text.indexOf(startRe));
        while (start >= 0) {
            const auto end = endRe.match(text, start);
            int len = 0;
            if (!end.hasMatch()) {
                setCurrentBlockState(1);
                len = text.length() - start;
            } else {
                len = static_cast<int>(end.capturedEnd()) - start;
            }
            setFormat(start, len, commentFmt_);
            const auto next = startRe.match(text, start + len);
            start = next.hasMatch() ? static_cast<int>(next.capturedStart()) : -1;
        }
    }

private:
    struct Rule {
        QRegularExpression re;
        QTextCharFormat fmt;
    };
    std::vector<Rule> rules_;
    QTextCharFormat strFmt_;
    QTextCharFormat commentFmt_;
    bool blockComments_ = false;
};

CodeHighlighter::CodeHighlighter(QObject* parent) : QObject(parent) {}
CodeHighlighter::~CodeHighlighter() = default;

void CodeHighlighter::setDocument(QQuickTextDocument* d) {
    if (doc_ == d) {
        return;
    }
    doc_ = d;
    emit documentChanged();
    rebuild();
}

void CodeHighlighter::setLanguage(const QString& lang) {
    if (lang_ == lang) {
        return;
    }
    lang_ = lang;
    emit languageChanged();
    rebuild();
}

void CodeHighlighter::setEnabled(bool v) {
    if (enabled_ == v) {
        return;
    }
    enabled_ = v;
    emit enabledChanged();
    rebuild();
}

void CodeHighlighter::rebuild() {
    hl_.reset(); // solta o highlighter antigo (desattacha do documento)
    if (doc_ == nullptr || !enabled_) {
        return;
    }
    if (QTextDocument* td = doc_->textDocument()) {
        hl_ = std::make_unique<Impl>(td, lang_);
    }
}

} // namespace dante
