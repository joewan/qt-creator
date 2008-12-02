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
#include "pathchooser.h"
#include "basevalidatinglineedit.h"

#include <QtGui/QLineEdit>
#include <QtGui/QHBoxLayout>
#include <QtGui/QToolButton>
#include <QtGui/QFileDialog>
#include <QtGui/QDesktopServices>

#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtCore/QSettings>
#include <QtCore/QDebug>

namespace Core {
namespace Utils {

// ------------------ PathValidatingLineEdit
class PathValidatingLineEdit : public BaseValidatingLineEdit {
public:
    explicit PathValidatingLineEdit(QWidget *parent = 0);

protected:
    virtual bool validate(const QString &value, QString *errorMessage) const;
};

PathValidatingLineEdit::PathValidatingLineEdit(QWidget *parent) :
    BaseValidatingLineEdit(parent)
{
}

bool PathValidatingLineEdit::validate(const QString &value, QString *errorMessage) const
{
    return PathChooser::validatePath(value, errorMessage);
}

// ------------------ PathChooserPrivate
struct PathChooserPrivate {
    PathChooserPrivate();

    PathValidatingLineEdit *m_lineEdit;
};

PathChooserPrivate::PathChooserPrivate() :
    m_lineEdit(new PathValidatingLineEdit)
{
}

PathChooser::PathChooser(QWidget *parent) :
    QWidget(parent),
    m_d(new PathChooserPrivate)
{
    QHBoxLayout *hLayout = new QHBoxLayout;
    hLayout->setContentsMargins(0, 0, 0, 0);

    connect(m_d->m_lineEdit, SIGNAL(validReturnPressed()), this, SIGNAL(returnPressed()));
    connect(m_d->m_lineEdit, SIGNAL(textChanged(QString)), this, SIGNAL(changed()));
    connect(m_d->m_lineEdit, SIGNAL(validChanged()), this, SIGNAL(validChanged()));

    m_d->m_lineEdit->setMinimumWidth(300);
    hLayout->addWidget(m_d->m_lineEdit);

    QToolButton *browseButton = new QToolButton;
    browseButton->setText(tr("..."));
    connect(browseButton, SIGNAL(clicked()), this, SLOT(slotBrowse()));

    hLayout->addWidget(browseButton);
    setLayout(hLayout);
    setFocusProxy(m_d->m_lineEdit);
}

PathChooser::~PathChooser()
{
    delete m_d;
}

QString PathChooser::path() const
{
    return m_d->m_lineEdit->text();
}

void PathChooser::setPath(const QString &path)
{
    const QString defaultPath = path.isEmpty() ? homePath() : path;
    m_d->m_lineEdit->setText(QDir::toNativeSeparators(defaultPath));
}

void PathChooser::slotBrowse()
{
    QString predefined = path();
    if (!predefined.isEmpty() && !QFileInfo(predefined).isDir())
        predefined.clear();
    // Prompt for a directory, delete trailing slashes unless it is "/", only
    QString newPath = QFileDialog::getExistingDirectory(this, tr("Choose a path"), predefined);
    if (!newPath .isEmpty()) {
        if (newPath .size() > 1 && newPath .endsWith(QDir::separator()))
            newPath .truncate(newPath .size() - 1);
        setPath(newPath);
    }
}

bool PathChooser::isValid() const
{
    return m_d->m_lineEdit->isValid();
}

QString PathChooser::errorMessage() const
{
    return  m_d->m_lineEdit->errorMessage();
}

bool PathChooser::validatePath(const QString &path, QString *errorMessage)
{
    if (path.isEmpty()) {
        if (errorMessage)
            *errorMessage = tr("The path must not be empty.");
        return false;
    }
    // Must be a directory?
    const QFileInfo fi(path);
    if (fi.isDir())
        return true; // Happy!

    if (!fi.exists()) {
        if (errorMessage)
            *errorMessage = tr("The path '%1' does not exist.").arg(path);
        return false;
    }
    // Must be something weird
    if (errorMessage)
        *errorMessage = tr("The path '%1' is not a directory.").arg(path);
    return false;
}

QString PathChooser::label()
{
    return tr("Path:");
}

QString PathChooser::homePath()
{
#ifdef Q_OS_WIN
    // Return 'users/<name>/Documents' on Windows, since Windows explorer
    // does not let people actually display the contents of their home
    // directory. Alternatively, create a QtCreator-specific directory?
    return QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation);
#else
    return QDir::homePath();
#endif
}

}
}
