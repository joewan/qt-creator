/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "cpplocatorfilter.h"
#include "cppmodelmanager.h"

#include <clangwrapper/indexedsymbolinfo.h>

#include <texteditor/itexteditor.h>
#include <texteditor/basetexteditor.h>

#include <QtCore/QStringMatcher>

using namespace CppTools::Internal;

CppLocatorFilter::CppLocatorFilter(CppModelManager *manager)
    : m_manager(manager),
    m_forceNewSearchList(true)
{
    setShortcutString(QString(QLatin1Char(':')));
    setIncludedByDefault(false);

    connect(manager, SIGNAL(documentUpdated(CPlusPlus::Document::Ptr)),
            this, SLOT(onDocumentUpdated(CPlusPlus::Document::Ptr)));

    connect(manager, SIGNAL(aboutToRemoveFiles(QStringList)),
            this, SLOT(onAboutToRemoveFiles(QStringList)));
}

CppLocatorFilter::~CppLocatorFilter()
{ }

void CppLocatorFilter::onDocumentUpdated(CPlusPlus::Document::Ptr doc)
{
    // Testing clang indexer, so this is commented out for now...
    Q_UNUSED(doc);
    //m_searchList[doc->fileName()] = search(doc);
}

void CppLocatorFilter::onAboutToRemoveFiles(const QStringList &files)
{
    foreach (const QString &file, files)
        m_searchList.remove(file);
}

void CppLocatorFilter::refresh(QFutureInterface<void> &future)
{
    Q_UNUSED(future)
}

static bool compareLexigraphically(const Locator::FilterEntry &a,
                                   const Locator::FilterEntry &b)
{
    return a.displayName < b.displayName;
}

QList<Locator::FilterEntry> CppLocatorFilter::matchesFor(QFutureInterface<Locator::FilterEntry> &future, const QString &origEntry)
{
    QString entry = trimWildcards(origEntry);
    QList<Locator::FilterEntry> goodEntries;
    QList<Locator::FilterEntry> betterEntries;
    const QChar asterisk = QLatin1Char('*');
    QStringMatcher matcher(entry, Qt::CaseInsensitive);
    const QRegExp regexp(asterisk + entry+ asterisk, Qt::CaseInsensitive, QRegExp::Wildcard);
    if (!regexp.isValid())
        return goodEntries;
    bool hasWildcard = (entry.contains(asterisk) || entry.contains('?'));



    // @TODO: Testing indexing with clang...
    const QString &shortcut = shortcutString();
    const QStringList &allFiles = m_manager->indexer().allFiles();
    foreach (const QString &file, allFiles) {
        if (future.isCanceled())
            break;

        QList<Clang::IndexedSymbolInfo> infoList;
        if (shortcut == QLatin1String("c")) {
            infoList.append(m_manager->indexer().classesFromFile(file));
        } else if (shortcut == QLatin1String("m")) {
            infoList.append(m_manager->indexer().functionsFromFile(file));
            infoList.append(m_manager->indexer().methodsFromFile(file));
        } else if (shortcut == QLatin1String(":")) {
            infoList.append(m_manager->indexer().classesFromFile(file));
            infoList.append(m_manager->indexer().methodsFromFile(file));
        }
        foreach (const Clang::IndexedSymbolInfo &symbolInfo, infoList) {
            // @TODO: Merge ModelItemInfo and IndexedSymbomlInfo since they are pretty much
            // the same thing.
            ModelItemInfo info(symbolInfo.m_name,
                               symbolInfo.m_qualification,
                               ModelItemInfo::ItemType((int)symbolInfo.m_type),
                               QStringList(QLatin1String("qual. name")),
                               symbolInfo.m_location.fileName(),
                               symbolInfo.m_location.line(),
                               symbolInfo.m_location.column() - 1, // @TODO: Column position...
                               symbolInfo.m_icon);

            if ((hasWildcard && regexp.exactMatch(info.symbolName))
                    || (!hasWildcard && matcher.indexIn(info.symbolName) != -1)) {

                QVariant id = qVariantFromValue(info);
                Locator::FilterEntry filterEntry(this, info.symbolName, id, info.icon);
                if (! info.symbolType.isEmpty())
                    filterEntry.extraInfo = info.symbolType;
                else
                    filterEntry.extraInfo = info.fileName;

                if (info.symbolName.startsWith(entry))
                    betterEntries.append(filterEntry);
                else
                    goodEntries.append(filterEntry);
            }
        }
    }

#if 0
    QHashIterator<QString, QList<ModelItemInfo> > it(m_searchList);
    while (it.hasNext()) {
        if (future.isCanceled())
            break;

        it.next();

        const QList<ModelItemInfo> items = it.value();
        foreach (const ModelItemInfo &info, items) {
            if ((hasWildcard && regexp.exactMatch(info.symbolName))
                    || (!hasWildcard && matcher.indexIn(info.symbolName) != -1)) {

                QVariant id = qVariantFromValue(info);
                Locator::FilterEntry filterEntry(this, info.symbolName, id, info.icon);
                if (! info.symbolType.isEmpty())
                    filterEntry.extraInfo = info.symbolType;
                else
                    filterEntry.extraInfo = info.fileName;

                if (info.symbolName.startsWith(entry))
                    betterEntries.append(filterEntry);
                else
                    goodEntries.append(filterEntry);
            }
        }
    }
#endif

    if (goodEntries.size() < 1000)
        qSort(goodEntries.begin(), goodEntries.end(), compareLexigraphically);
    if (betterEntries.size() < 1000)
        qSort(betterEntries.begin(), betterEntries.end(), compareLexigraphically);

    betterEntries += goodEntries;
    return betterEntries;
}

void CppLocatorFilter::accept(Locator::FilterEntry selection) const
{
    ModelItemInfo info = qvariant_cast<CppTools::Internal::ModelItemInfo>(selection.internalData);
    TextEditor::BaseTextEditorWidget::openEditorAt(info.fileName, info.line, info.column,
                                             QString(), Core::EditorManager::ModeSwitch);
}

void CppLocatorFilter::reset()
{
    m_searchList.clear();
    m_previousResults.clear();
    m_previousEntry.clear();
    m_forceNewSearchList = true;
}
