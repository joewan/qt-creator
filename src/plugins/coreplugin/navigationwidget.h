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
#ifndef NAVIGATIONWIDGET_H
#define NAVIGATIONWIDGET_H

#include <QtGui/QWidget>
#include <QtGui/QComboBox>
#include <QtGui/QSplitter>
#include <QtGui/QToolBar>
#include <QtGui/QToolButton>
#include <coreplugin/minisplitter.h>

QT_BEGIN_NAMESPACE
class QSettings;
class QShortcut;
QT_END_NAMESPACE

namespace Core {

class INavigationWidgetFactory;
class IMode;
class ICommand;

namespace Internal {
class NavigationWidget;
}

class CORE_EXPORT NavigationWidgetPlaceHolder : public QWidget
{
    friend class Core::Internal::NavigationWidget;
    Q_OBJECT
public:
    NavigationWidgetPlaceHolder(Core::IMode *mode, QWidget *parent = 0);
    ~NavigationWidgetPlaceHolder();
    static NavigationWidgetPlaceHolder* current();
    void applyStoredSize(int width);
private slots:
    void currentModeAboutToChange(Core::IMode *);
private:
    Core::IMode *m_mode;
    static NavigationWidgetPlaceHolder* m_current;
};

namespace Internal {

class NavigationSubWidget;

class NavigationWidget : public MiniSplitter
{
    Q_OBJECT
public:
    NavigationWidget(QAction *toggleSideBarAction);
    ~NavigationWidget();

    void saveSettings(QSettings *settings);
    void readSettings(QSettings *settings);

    bool isShown() const;
    void setShown(bool b);

    bool isSuppressed() const;
    void setSuppressed(bool b);

    static NavigationWidget* instance();

    int storedWidth();

    // Called from the place holders
    void placeHolderChanged(NavigationWidgetPlaceHolder *holder);

    QHash<QString, Core::ICommand*> commandMap() const { return m_commandMap; }

protected:
    void resizeEvent(QResizeEvent *);
private slots:
    void objectAdded(QObject*);
    void activateSubWidget();
    void split();
    void close();

private:
    NavigationSubWidget *insertSubItem(int position);
    QList<NavigationSubWidget *> m_subWidgets;
    QHash<QShortcut *, QString> m_shortcutMap;
    QHash<QString, Core::ICommand*> m_commandMap;
    bool m_shown;
    bool m_suppressed;
    int m_width;
    static NavigationWidget* m_instance;
    QAction *m_toggleSideBarAction;
};

class NavigationSubWidget : public QWidget
{
    Q_OBJECT
public:
    NavigationSubWidget(NavigationWidget *parentWidget);
    virtual ~NavigationSubWidget();

    INavigationWidgetFactory *factory();
    void setFactory(INavigationWidgetFactory *factory);
    void setFactory(const QString &name);
    void setFocusWidget();

    Core::ICommand *command(const QString &title) const;

signals:
    void split();
    void close();

private slots:
    void objectAdded(QObject*);
    void aboutToRemoveObject(QObject*);
    void setCurrentIndex(int);

private:
    NavigationWidget *m_parentWidget;
    QComboBox *m_navigationComboBox;
    QWidget *m_navigationWidget;
    QToolBar *m_toolbar;
    QAction *m_splitAction;
    QList<QToolButton *> m_additionalToolBarWidgets;
};

class NavComboBox : public QComboBox
{
    Q_OBJECT

public:
    NavComboBox(NavigationSubWidget *navSubWidget);

protected:
    bool event(QEvent *event);

private:
    NavigationSubWidget *m_navSubWidget;
};

} // namespace Internal
} // namespace Core

#endif // NAVIGATIONWIDGET_H
