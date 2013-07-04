/**************************************************************************
**
** Copyright (c) 2013 Denis Mingulov
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "classviewsymbollocation.h"
#include <QPair>
#include <QHash>

namespace ClassView {
namespace Internal {

/*!
    \class SymbolLocation
    \brief The SymbolLocation class stores information about symbol location
    to know the exact location to open when the user clicks on a tree item.

    This class might be used in QSet and QHash.
*/

SymbolLocation::SymbolLocation() :
    m_line(0),
    m_column(0),
    m_hash(0)
{
}

SymbolLocation::SymbolLocation(QString file, int lineNumber, int columnNumber) :
    m_fileName(file),
    m_line(lineNumber),
    m_column(columnNumber)
{
    if (m_column < 0)
        m_column = 0;

    // pre-computate hash value
    m_hash = qHash(qMakePair(m_fileName, qMakePair(m_line, m_column)));
}

} // namespace Internal
} // namespace ClassView
