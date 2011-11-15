/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "cpphighlighter.h"
#include <cpptools/cppdoxygen.h>

#include <Token.h>
#include <cplusplus/SimpleLexer.h>
#include <texteditor/basetextdocumentlayout.h>

#include <QtGui/QTextDocument>
#include <QtCore/QDebug>

using namespace CppEditor::Internal;
using namespace TextEditor;
using namespace CPlusPlus;
using namespace Clang;

CppHighlighter::CppHighlighter(QTextDocument *document) :
    TextEditor::SyntaxHighlighter(document)
{
    m_lexer.includeC99();
    m_lexer.includeCpp0x();
    m_lexer.includeCppOperators();
    m_lexer.init();
}

void CppHighlighter::highlightBlock(const QString &text)
{
    const int previousState = previousBlockState();
    int state = 0, initialBraceDepth = 0;
    if (previousState != -1) {
        state = previousState & 0xff;
        initialBraceDepth = previousState >> 8;
    }

    int braceDepth = initialBraceDepth;

    int initialState = state;
    const QList<Clang::Token> &tokens = m_lexer.lex(text, &state);

    int foldingIndent = initialBraceDepth;
    if (TextBlockUserData *userData = BaseTextDocumentLayout::testUserData(currentBlock())) {
        userData->setFoldingIndent(0);
        userData->setFoldingStartIncluded(false);
        userData->setFoldingEndIncluded(false);
    }

    if (tokens.isEmpty()) {
        setCurrentBlockState(previousState);
        BaseTextDocumentLayout::clearParentheses(currentBlock());
        if (text.length()) // the empty line can still contain whitespace
            setFormat(0, text.length(), m_formats[CppVisualWhitespace]);
        BaseTextDocumentLayout::setFoldingIndent(currentBlock(), foldingIndent);
        return;
    }

    const unsigned firstNonSpace = tokens.first().begin();

    Parentheses parentheses;
    parentheses.reserve(20); // assume wizard level ;-)

    bool expectPreprocessorKeyword = false;
    bool onlyHighlightComments = false;

    for (int i = 0; i < tokens.size(); ++i) {
        const Clang::Token &tk = tokens.at(i);

        unsigned previousTokenEnd = 0;
        if (i != 0) {
            // mark the whitespaces
            previousTokenEnd = tokens.at(i - 1).begin() +
                               tokens.at(i - 1).length();
        }

        if (previousTokenEnd != tk.begin()) {
            setFormat(previousTokenEnd, tk.begin() - previousTokenEnd,
                      m_formats[CppVisualWhitespace]);
        }

        if (tk.is(Clang::Token::Punctuation)) {
            const QStringRef &punc = text.midRef(tk.begin(), tk.length());
            if (punc.length() == 1) {
                if (Clang::Token::isPunctuationLBrace(tk, text)
                        || Clang::Token::isPunctuationLParen(tk, text)
                        || Clang::Token::isPunctuationLBracked(tk, text)) {
                    parentheses.append(Parenthesis(Parenthesis::Opened, punc.at(0), tk.begin()));
                    if (Clang::Token::isPunctuationLBrace(tk, text)) {
                        ++braceDepth;

                        // if a folding block opens at the beginning of a line, treat the entire line
                        // as if it were inside the folding block
                        if (tk.begin() == firstNonSpace) {
                            ++foldingIndent;
                            BaseTextDocumentLayout::userData(currentBlock())->setFoldingStartIncluded(true);
                        }
                    }
                } else if (Clang::Token::isPunctuationRBrace(tk, text)
                           || Clang::Token::isPunctuationRBracked(tk, text)
                           || Clang::Token::isPunctuationRParen(tk, text)) {
                    parentheses.append(Parenthesis(Parenthesis::Closed, punc.at(0), tk.begin()));
                    if (Clang::Token::isPunctuationRBrace(tk, text)) {
                        --braceDepth;
                        if (braceDepth < foldingIndent) {
                            // unless we are at the end of the block, we reduce the folding indent
                            if (i == tokens.size()-1
                                    || (tokens.at(i+1).is(Clang::Token::Punctuation)
                                        && Clang::Token::isPunctuationSemiColon(tk, text))) {
                                BaseTextDocumentLayout::userData(currentBlock())->setFoldingEndIncluded(true);
                            } else {
                                foldingIndent = qMin(braceDepth, foldingIndent);
                            }
                        }
                    }
                }
            }
        }

        bool highlightCurrentWordAsPreprocessor = expectPreprocessorKeyword;

        if (expectPreprocessorKeyword)
            expectPreprocessorKeyword = false;

        if (onlyHighlightComments && !tk.is(Clang::Token::Comment))
            continue;


        if (i == 0
                && tk.is(Clang::Token::Punctuation)
                && Clang::Token::isPunctuationPound(tk, text)) {
            highlightLine(text, tk.begin(), tk.length(), m_formats[CppPreprocessorFormat]);
            expectPreprocessorKeyword = true;
        } else if (highlightCurrentWordAsPreprocessor
                   && (tk.is(Clang::Token::Keyword)
                       || tk.is(Clang::Token::Identifier))
                   && isPPKeyword(text.midRef(tk.begin(), tk.length()))) {
            setFormat(tk.begin(), tk.length(), m_formats[CppPreprocessorFormat]);
            const QStringRef ppKeyword = text.midRef(tk.begin(), tk.length());
            if (ppKeyword == QLatin1String("error")
                    || ppKeyword == QLatin1String("warning")
                    || ppKeyword == QLatin1String("pragma")) {
                onlyHighlightComments = true;
            }
        } else if (tk.is(Clang::Token::Literal)) {
            if (Clang::Token::isLiteralNumeric(tk, text))
                setFormat(tk.begin(), tk.length(), m_formats[CppNumberFormat]);
            else
                highlightLine(text, tk.begin(), tk.length(), m_formats[CppStringFormat]);
        } else if (tk.is(Clang::Token::Comment)) {
            if (tk.isDoxygenComment())
                highlightDoxygenComment(text, tk.begin(), tk.length());
            else
                highlightLine(text, tk.begin(), tk.length(), m_formats[CppCommentFormat]);

            // we need to insert a close comment parenthesis, if
            //  - the line starts in a C Comment (initalState != 0)
            //  - the first token of the line is a T_COMMENT (i == 0 && tk.is(T_COMMENT))
            //  - is not a continuation line (tokens.size() > 1 || ! state)
            if (initialState && i == 0 && (tokens.size() > 1 || ! state)) {
                --braceDepth;
                // unless we are at the end of the block, we reduce the folding indent
                if (i == tokens.size()-1)
                    BaseTextDocumentLayout::userData(currentBlock())->setFoldingEndIncluded(true);
                else
                    foldingIndent = qMin(braceDepth, foldingIndent);
                const int tokenEnd = tk.begin() + tk.length() - 1;
                parentheses.append(Parenthesis(Parenthesis::Closed, QLatin1Char('-'), tokenEnd));

                // clear the initial state.
                initialState = 0;
            }
        } else if (tk.is(Clang::Token::Keyword)) {
            setFormat(tk.begin(), tk.length(), m_formats[CppKeywordFormat]);
        } else if (i == 0
                   && tokens.size() > 1
                   && tk.is(Clang::Token::Identifier)
                   && tokens.at(1).is(Clang::Token::Punctuation)
                   && Clang::Token::isPunctuationColon(tokens.at(1), text)) {
            setFormat(tk.begin(), tk.length(), m_formats[CppLabelFormat]);
        } else if (tk.is(Clang::Token::Identifier)) {
            highlightWord(text.midRef(tk.begin(), tk.length()), tk.begin(), tk.length());
        } else if (tk.is(Clang::Token::Punctuation)){
            setFormat(tk.begin(), tk.length(), m_formats[CppOperatorFormat]);
        }
    }

    // mark the trailing white spaces
    {
        const Clang::Token tk = tokens.last();
        const int lastTokenEnd = tk.begin() + tk.length();
        if (text.length() > lastTokenEnd)
            highlightLine(text, lastTokenEnd, text.length() - lastTokenEnd, QTextCharFormat());
    }

    if (! initialState && state && ! tokens.isEmpty()) {
        parentheses.append(Parenthesis(Parenthesis::Opened, QLatin1Char('+'),
                                       tokens.last().begin()));
        ++braceDepth;
    }

    BaseTextDocumentLayout::setParentheses(currentBlock(), parentheses);

    // if the block is ifdefed out, we only store the parentheses, but
    // do not adjust the brace depth.
    if (BaseTextDocumentLayout::ifdefedOut(currentBlock())) {
        braceDepth = initialBraceDepth;
        foldingIndent = initialBraceDepth;
    }

    BaseTextDocumentLayout::setFoldingIndent(currentBlock(), foldingIndent);

    // optimization: if only the brace depth changes, we adjust subsequent blocks
    // to have QSyntaxHighlighter stop the rehighlighting
    int currentState = currentBlockState();
    if (currentState != -1) {
        int oldState = currentState & 0xff;
        int oldBraceDepth = currentState >> 8;

        ///checar state
        if (oldState == state && oldBraceDepth != braceDepth) {
            int delta = braceDepth - oldBraceDepth;
            QTextBlock block = currentBlock().next();
            while (block.isValid() && block.userState() != -1) {
                BaseTextDocumentLayout::changeBraceDepth(block, delta);
                BaseTextDocumentLayout::changeFoldingIndent(block, delta);
                block = block.next();
            }
        }
    }

    setCurrentBlockState((braceDepth << 8) | state);
}

#if 0
void CppHighlighter::highlightBlock(const QString &text)
{
    const int previousState = previousBlockState();
    int state = 0, initialBraceDepth = 0;
    if (previousState != -1) {
        state = previousState & 0xff;
        initialBraceDepth = previousState >> 8;
    }

    int braceDepth = initialBraceDepth;

    SimpleLexer tokenize;
    tokenize.setQtMocRunEnabled(false);
    tokenize.setObjCEnabled(false);

    int initialState = state;
    const QList<Token> tokens = tokenize(text, initialState);
    state = tokenize.state(); // refresh the state

    int foldingIndent = initialBraceDepth;
    if (TextBlockUserData *userData = BaseTextDocumentLayout::testUserData(currentBlock())) {
        userData->setFoldingIndent(0);
        userData->setFoldingStartIncluded(false);
        userData->setFoldingEndIncluded(false);
    }

    if (tokens.isEmpty()) {
        setCurrentBlockState(previousState);
        BaseTextDocumentLayout::clearParentheses(currentBlock());
        if (text.length()) // the empty line can still contain whitespace
            setFormat(0, text.length(), m_formats[CppVisualWhitespace]);
        BaseTextDocumentLayout::setFoldingIndent(currentBlock(), foldingIndent);
        return;
    }

    const unsigned firstNonSpace = tokens.first().begin();

    Parentheses parentheses;
    parentheses.reserve(20); // assume wizard level ;-)

    bool expectPreprocessorKeyword = false;
    bool onlyHighlightComments = false;

    for (int i = 0; i < tokens.size(); ++i) {
        const Token &tk = tokens.at(i);

        unsigned previousTokenEnd = 0;
        if (i != 0) {
            // mark the whitespaces
            previousTokenEnd = tokens.at(i - 1).begin() +
                               tokens.at(i - 1).length();
        }

        if (previousTokenEnd != tk.begin()) {
            setFormat(previousTokenEnd, tk.begin() - previousTokenEnd,
                      m_formats[CppVisualWhitespace]);
        }

        if (tk.is(T_LPAREN) || tk.is(T_LBRACE) || tk.is(T_LBRACKET)) {
            const QChar c = text.at(tk.begin());
            parentheses.append(Parenthesis(Parenthesis::Opened, c, tk.begin()));
            if (tk.is(T_LBRACE)) {
                ++braceDepth;

                // if a folding block opens at the beginning of a line, treat the entire line
                // as if it were inside the folding block
                if (tk.begin() == firstNonSpace) {
                    ++foldingIndent;
                    BaseTextDocumentLayout::userData(currentBlock())->setFoldingStartIncluded(true);
                }
            }
        } else if (tk.is(T_RPAREN) || tk.is(T_RBRACE) || tk.is(T_RBRACKET)) {
            const QChar c = text.at(tk.begin());
            parentheses.append(Parenthesis(Parenthesis::Closed, c, tk.begin()));
            if (tk.is(T_RBRACE)) {
                --braceDepth;
                if (braceDepth < foldingIndent) {
                    // unless we are at the end of the block, we reduce the folding indent
                    if (i == tokens.size()-1 || tokens.at(i+1).is(T_SEMICOLON))
                        BaseTextDocumentLayout::userData(currentBlock())->setFoldingEndIncluded(true);
                    else
                        foldingIndent = qMin(braceDepth, foldingIndent);
                }
            }
        }

        bool highlightCurrentWordAsPreprocessor = expectPreprocessorKeyword;

        if (expectPreprocessorKeyword)
            expectPreprocessorKeyword = false;

        if (onlyHighlightComments && !tk.isComment())
            continue;

        if (i == 0 && tk.is(T_POUND)) {
            highlightLine(text, tk.begin(), tk.length(), m_formats[CppPreprocessorFormat]);
            expectPreprocessorKeyword = true;
        } else if (highlightCurrentWordAsPreprocessor &&
                   (tk.isKeyword() || tk.is(T_IDENTIFIER)) && isPPKeyword(text.midRef(tk.begin(), tk.length()))) {
            setFormat(tk.begin(), tk.length(), m_formats[CppPreprocessorFormat]);
            const QStringRef ppKeyword = text.midRef(tk.begin(), tk.length());
            if (ppKeyword == QLatin1String("error")
                    || ppKeyword == QLatin1String("warning")
                    || ppKeyword == QLatin1String("pragma")) {
                onlyHighlightComments = true;
            }

        } else if (tk.is(T_NUMERIC_LITERAL))
            setFormat(tk.begin(), tk.length(), m_formats[CppNumberFormat]);

        else if (tk.is(T_STRING_LITERAL) || tk.is(T_CHAR_LITERAL) || tk.is(T_ANGLE_STRING_LITERAL) ||
                 tk.is(T_AT_STRING_LITERAL))
            highlightLine(text, tk.begin(), tk.length(), m_formats[CppStringFormat]);

        else if (tk.is(T_WIDE_STRING_LITERAL) || tk.is(T_WIDE_CHAR_LITERAL))
            highlightLine(text, tk.begin(), tk.length(), m_formats[CppStringFormat]);

        else if (tk.isComment()) {

            if (tk.is(T_COMMENT) || tk.is(T_CPP_COMMENT))
                highlightLine(text, tk.begin(), tk.length(), m_formats[CppCommentFormat]);

            else // a doxygen comment
                highlightDoxygenComment(text, tk.begin(), tk.length());

            // we need to insert a close comment parenthesis, if
            //  - the line starts in a C Comment (initalState != 0)
            //  - the first token of the line is a T_COMMENT (i == 0 && tk.is(T_COMMENT))
            //  - is not a continuation line (tokens.size() > 1 || ! state)
            if (initialState && i == 0 && (tokens.size() > 1 || ! state)) {
                --braceDepth;
                // unless we are at the end of the block, we reduce the folding indent
                if (i == tokens.size()-1)
                    BaseTextDocumentLayout::userData(currentBlock())->setFoldingEndIncluded(true);
                else
                    foldingIndent = qMin(braceDepth, foldingIndent);
                const int tokenEnd = tk.begin() + tk.length() - 1;
                parentheses.append(Parenthesis(Parenthesis::Closed, QLatin1Char('-'), tokenEnd));

                // clear the initial state.
                initialState = 0;
            }

        } else if (tk.isKeyword() || isQtKeyword(text.midRef(tk.begin(), tk.length())) || tk.isObjCAtKeyword())
            setFormat(tk.begin(), tk.length(), m_formats[CppKeywordFormat]);

        else if (tk.isOperator())
            setFormat(tk.begin(), tk.length(), m_formats[CppOperatorFormat]);

        else if (i == 0 && tokens.size() > 1 && tk.is(T_IDENTIFIER) && tokens.at(1).is(T_COLON))
            setFormat(tk.begin(), tk.length(), m_formats[CppLabelFormat]);

        else if (tk.is(T_IDENTIFIER))
            highlightWord(text.midRef(tk.begin(), tk.length()), tk.begin(), tk.length());

    }

    // mark the trailing white spaces
    {
        const Token tk = tokens.last();
        const int lastTokenEnd = tk.begin() + tk.length();
        if (text.length() > lastTokenEnd)
            highlightLine(text, lastTokenEnd, text.length() - lastTokenEnd, QTextCharFormat());
    }

    if (! initialState && state && ! tokens.isEmpty()) {
        parentheses.append(Parenthesis(Parenthesis::Opened, QLatin1Char('+'),
                                       tokens.last().begin()));
        ++braceDepth;
    }

    BaseTextDocumentLayout::setParentheses(currentBlock(), parentheses);

    // if the block is ifdefed out, we only store the parentheses, but

    // do not adjust the brace depth.
    if (BaseTextDocumentLayout::ifdefedOut(currentBlock())) {
        braceDepth = initialBraceDepth;
        foldingIndent = initialBraceDepth;
    }

    BaseTextDocumentLayout::setFoldingIndent(currentBlock(), foldingIndent);

    // optimization: if only the brace depth changes, we adjust subsequent blocks
    // to have QSyntaxHighlighter stop the rehighlighting
    int currentState = currentBlockState();
    if (currentState != -1) {
        int oldState = currentState & 0xff;
        int oldBraceDepth = currentState >> 8;
        if (oldState == tokenize.state() && oldBraceDepth != braceDepth) {
            BaseTextDocumentLayout::FoldValidator foldValidor;
            foldValidor.setup(qobject_cast<BaseTextDocumentLayout *>(document()->documentLayout()));
            int delta = braceDepth - oldBraceDepth;
            QTextBlock block = currentBlock().next();
            while (block.isValid() && block.userState() != -1) {
                BaseTextDocumentLayout::changeBraceDepth(block, delta);
                BaseTextDocumentLayout::changeFoldingIndent(block, delta);
                foldValidor.process(block);
                block = block.next();
            }
            foldValidor.finalize();
        }
    }

    setCurrentBlockState((braceDepth << 8) | tokenize.state());
}
#endif

bool CppHighlighter::isPPKeyword(const QStringRef &text) const
{
    switch (text.length())
    {
    case 2:
        if (text.at(0) == 'i' && text.at(1) == 'f')
            return true;
        break;

    case 4:
        if (text.at(0) == 'e' && text == QLatin1String("elif"))
            return true;
        else if (text.at(0) == 'e' && text == QLatin1String("else"))
            return true;
        break;

    case 5:
        if (text.at(0) == 'i' && text == QLatin1String("ifdef"))
            return true;
        else if (text.at(0) == 'u' && text == QLatin1String("undef"))
            return true;
        else if (text.at(0) == 'e' && text == QLatin1String("endif"))
            return true;
        else if (text.at(0) == 'e' && text == QLatin1String("error"))
            return true;
        break;

    case 6:
        if (text.at(0) == 'i' && text == QLatin1String("ifndef"))
            return true;
        if (text.at(0) == 'i' && text == QLatin1String("import"))
            return true;
        else if (text.at(0) == 'd' && text == QLatin1String("define"))
            return true;
        else if (text.at(0) == 'p' && text == QLatin1String("pragma"))
            return true;
        break;

    case 7:
        if (text.at(0) == 'i' && text == QLatin1String("include"))
            return true;
        else if (text.at(0) == 'w' && text == QLatin1String("warning"))
            return true;
        break;

    case 12:
        if (text.at(0) == 'i' && text == QLatin1String("include_next"))
            return true;
        break;

    default:
        break;
    }

    return false;
}

bool CppHighlighter::isQtKeyword(const QStringRef &text) const
{
    switch (text.length()) {
    case 4:
        if (text.at(0) == 'e' && text == QLatin1String("emit"))
            return true;
        else if (text.at(0) == 'S' && text == QLatin1String("SLOT"))
            return true;
        break;

    case 5:
        if (text.at(0) == 's' && text == QLatin1String("slots"))
            return true;
        break;

    case 6:
        if (text.at(0) == 'S' && text == QLatin1String("SIGNAL"))
            return true;
        break;

    case 7:
        if (text.at(0) == 's' && text == QLatin1String("signals"))
            return true;
        else if (text.at(0) == 'f' && text == QLatin1String("foreach"))
            return true;
        else if (text.at(0) == 'f' && text == QLatin1String("forever"))
            return true;
        break;

    default:
        break;
    }
    return false;
}

void CppHighlighter::highlightLine(const QString &text, int position, int length,
                                   const QTextCharFormat &format)
{
    const QTextCharFormat visualSpaceFormat = m_formats[CppVisualWhitespace];

    const int end = position + length;
    int index = position;

    while (index != end) {
        const bool isSpace = text.at(index).isSpace();
        const int start = index;

        do { ++index; }
        while (index != end && text.at(index).isSpace() == isSpace);

        const int tokenLength = index - start;
        if (isSpace)
            setFormat(start, tokenLength, visualSpaceFormat);
        else if (format.isValid())
            setFormat(start, tokenLength, format);
    }
}

void CppHighlighter::highlightWord(QStringRef word, int position, int length)
{
    // try to highlight Qt 'identifiers' like QObject and Q_PROPERTY

    if (word.length() > 2 && word.at(0) == QLatin1Char('Q')) {
        if (word.at(1) == QLatin1Char('_') // Q_
            || (word.at(1) == QLatin1Char('T') && word.at(2) == QLatin1Char('_'))) { // QT_
            for (int i = 1; i < word.length(); ++i) {
                const QChar &ch = word.at(i);
                if (! (ch.isUpper() || ch == QLatin1Char('_')))
                    return;
            }

            setFormat(position, length, m_formats[CppTypeFormat]);
        }
    }
}

void CppHighlighter::highlightDoxygenComment(const QString &text, int position, int)
{
    int initial = position;

    const QChar *uc = text.unicode();
    const QChar *it = uc + position;

    const QTextCharFormat &format = m_formats[CppDoxygenCommentFormat];
    const QTextCharFormat &kwFormat = m_formats[CppDoxygenTagFormat];

    while (! it->isNull()) {
        if (it->unicode() == QLatin1Char('\\') ||
            it->unicode() == QLatin1Char('@')) {
            ++it;

            const QChar *start = it;
            while (it->isLetterOrNumber() || it->unicode() == '_')
                ++it;

            int k = CppTools::classifyDoxygenTag(start, it - start);
            if (k != CppTools::T_DOXY_IDENTIFIER) {
                highlightLine(text, initial, start - uc - initial, format);
                setFormat(start - uc - 1, it - start + 1, kwFormat);
                initial = it - uc;
            }
        } else
            ++it;
    }

    highlightLine(text, initial, it - uc - initial, format);
}

