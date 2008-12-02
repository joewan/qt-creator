/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** 
** Non-Open Source Usage  
** 
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.  
** 
** GNU General Public License Usage 
** 
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception version
** 1.2, included in the file GPL_EXCEPTION.txt in this package.  
** 
***************************************************************************/
#ifndef BASETEXTEDITOR_P_H
#define BASETEXTEDITOR_P_H

#include "basetexteditor.h"

#include <QtCore/QBasicTimer>
#include <QtCore/QTimeLine>
#include <QtCore/QSharedData>

#include <QtGui/QTextEdit>
#include <QtGui/QPixmap>

namespace TextEditor {

class BaseTextDocument;

namespace Internal {

//========== Pointers with reference count ==========

template <class T> class QRefCountData : public QSharedData
{
public:
    QRefCountData(T *data) { m_data = data; }

    ~QRefCountData() { delete m_data; }

    T *m_data;
};

/* MOSTLY COPIED FROM QSHAREDDATA(-POINTER) */
template <class T> class QRefCountPointer
{
public:
    inline T &operator*() { return d ? *(d->m_data) : 0; }
    inline const T &operator*() const { return d ? *(d->m_data) : 0; }
    inline T *operator->() { return d ? d->m_data : 0; }
    inline const T *operator->() const { return d ? d->m_data : 0; }
    inline operator T *() { return d ? d->m_data : 0; }
    inline operator const T *() const { return d ? d->m_data : 0; }

    inline bool operator==(const QRefCountPointer<T> &other) const { return d == other.d; }
    inline bool operator!=(const QRefCountPointer<T> &other) const { return d != other.d; }

    inline QRefCountPointer() { d = 0; }
    inline ~QRefCountPointer() { if (d && !d->ref.deref()) delete d; }

    explicit QRefCountPointer(T *data) {
        if (data) {
            d = new QRefCountData<T>(data);
            d->ref.ref();
        }
        else {
            d = 0;
        }
    }
    inline QRefCountPointer(const QRefCountPointer<T> &o) : d(o.d) { if (d) d->ref.ref(); }
    inline QRefCountPointer<T> & operator=(const QRefCountPointer<T> &o) {
        if (o.d != d) {
            if (d && !d->ref.deref())
                delete d;
            //todo: atomic assign of pointers
            d = o.d;
            if (d)
                d->ref.ref();
        }
        return *this;
    }
    inline QRefCountPointer &operator=(T *o) {
        if (d == 0 || d->m_data != o) {
            if (d && !d->ref.deref())
                delete d;
            d = new QRefCountData<T>(o);
            if (d)
                d->ref.ref();
        }
        return *this;
    }

    inline bool operator!() const { return !d; }

private:
    QRefCountData<T> *d;
};

//================BaseTextEditorPrivate==============

class BaseTextEditorPrivate
{
    BaseTextEditorPrivate(const BaseTextEditorPrivate &);
    BaseTextEditorPrivate &operator=(const BaseTextEditorPrivate &);

public:
    BaseTextEditorPrivate();
    ~BaseTextEditorPrivate();

#ifndef TEXTEDITOR_STANDALONE
    void setupBasicEditActions(TextEditorActionHandler *actionHandler);
#endif
    void setupDocumentSignals(BaseTextDocument *document);
    void updateLineSelectionColor();
#ifndef TEXTEDITOR_STANDALONE
    bool needMakeWritableCheck() const;
#endif

    void print(QPrinter *printer);

    QTextBlock m_firstVisible;
    int m_lastScrollPos;
    int m_lineNumber;

    BaseTextEditor *q;
    bool m_contentsChanged;

    QList<QTextEdit::ExtraSelection> m_syntaxHighlighterSelections;
    QTextEdit::ExtraSelection m_lineSelection;

    QRefCountPointer<BaseTextDocument> m_document;
    QByteArray m_tempState;

    QString m_displayName;
    bool m_parenthesesMatchingEnabled;
    QTimer *m_updateTimer;

    // parentheses matcher
    bool m_formatRange;
    QTextCharFormat m_matchFormat;
    QTextCharFormat m_mismatchFormat;
    QTextCharFormat m_rangeFormat;
    QTimer *m_parenthesesMatchingTimer;
    // end parentheses matcher

    QWidget *m_extraArea;
    DisplaySettings m_displaySettings;

    int extraAreaSelectionAnchorBlockNumber;
    int extraAreaToggleMarkBlockNumber;
    int extraAreaHighlightCollapseBlockNumber;
    int extraAreaCollapseAlpha;
    int extraAreaHighlightFadingBlockNumber;
    QTimeLine *extraAreaTimeLine;

    QBasicTimer collapsedBlockTimer;
    int visibleCollapsedBlockNumber;
    int suggestedVisibleCollapsedBlockNumber;
    void clearVisibleCollapsedBlock();

    QBasicTimer autoScrollTimer;
    void updateMarksLineNumber();
    void updateMarksBlock(const QTextBlock &block);
    uint m_marksVisible : 1;
    uint m_codeFoldingVisible : 1;
    uint m_revisionsVisible : 1;
    uint m_lineNumbersVisible : 1;
    uint m_highlightCurrentLine : 1;
    uint m_requestMarkEnabled : 1;
    uint m_lineSeparatorsAllowed : 1;
    int m_visibleWrapColumn;

    QTextCharFormat m_ifdefedOutFormat;

    QRegExp m_searchExpr;
    QTextDocument::FindFlags m_findFlags;
    QTextCharFormat m_searchResultFormat;
    QTextCharFormat m_searchScopeFormat;
    QTextCharFormat m_currentLineFormat;
    void highlightSearchResults(const QTextBlock &block,
                                QVector<QTextLayout::FormatRange> *selections);

    BaseTextEditorEditable *m_editable;

    QObject *m_actionHack;

    QList<QTextEdit::ExtraSelection> m_extraSelections;
    QList<QTextEdit::ExtraSelection> m_extraExtraSelections;

    // block selection mode
    bool m_inBlockSelectionMode;
    bool m_lastEventWasBlockSelectionEvent;
    int m_blockSelectionExtraX;
    void clearBlockSelection();
    QString copyBlockSelection();
    void removeBlockSelection(const QString &text = QString());
    
    QTextCursor m_findScope;

    void moveCursorVisible();
};

} // namespace Internal
} // namespace TextEditor

#endif // BASETEXTEDITOR_P_H
