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
#include <QtCore/QRegExp>

#include "pendingchangesdialog.h"

using namespace Perforce::Internal;

PendingChangesDialog::PendingChangesDialog(const QString &data, QWidget *parent)
    : QDialog(parent)
{
    m_ui.setupUi(this);
    if (!data.isEmpty()) {
        QRegExp r("Change\\s(\\d+).*\\s\\*pending\\*\\s(.+)\n");
        r.setMinimal(true);
        int pos = 0;
        QListWidgetItem *item;
        while ((pos = r.indexIn(data, pos)) != -1) {
            item = new QListWidgetItem(tr("Change %1: %2").arg(r.cap(1))
                .arg(r.cap(2).trimmed()), m_ui.listWidget);
            item->setData(234, r.cap(1).trimmed());
            ++pos;
        }
    }
    m_ui.listWidget->setSelectionMode(QListWidget::SingleSelection);
    if (m_ui.listWidget->count()) {
        m_ui.listWidget->setCurrentRow(0);
        m_ui.submitButton->setEnabled(true);
    } else {
        m_ui.submitButton->setEnabled(false);
    }
}

int PendingChangesDialog::changeNumber() const
{
    QListWidgetItem *item = m_ui.listWidget->item(m_ui.listWidget->currentRow());
    if (!item)
        return -1;
    bool ok = true;
    int i = item->data(234).toInt(&ok);
    return ok ? i : -1;
}
