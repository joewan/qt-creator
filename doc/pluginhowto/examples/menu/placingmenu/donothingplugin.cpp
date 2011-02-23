/***************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the documentation of Qt Creator.
**
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Nokia Corporation and its Subsidiary(-ies) nor
**     the names of its contributors may be used to endorse or promote
**     products derived from this software without specific prior written
**     permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
****************************************************************************/

#include "donothingplugin.h"
#include <coreplugin/coreconstants.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/icore.h>
#include <coreplugin/icontext.h>

#include <QKeySequence>
#include <QStringList>
#include <QMessageBox>
#include <QAction>
#include <QMenu>
#include <QtPlugin>

DoNothingPlugin::DoNothingPlugin()
{
    // Do nothing
}

DoNothingPlugin::~DoNothingPlugin()
{
    // Do notning
}

void DoNothingPlugin::extensionsInitialized()
{
    // Do nothing
}

bool DoNothingPlugin::initialize(const QStringList& args, QString *errMsg)
{
    Q_UNUSED(args);
    Q_UNUSED(errMsg);

    // Fetch the action manager
    Core::ActionManager* am = Core::ICore::instance()->actionManager();

    // Create a command for "DoNothing".
    QAction *action = new QAction(tr("DoNothing"),this);
    Core::Command* cmd = am->registerAction(action,
        QLatin1String("DoNothingPlugin.DoNothing"),
        Core::Context(Core::Constants::C_GLOBAL));

    // Add the "DoNothing" action to the tools menu
    am->actionContainer(Core::Constants::M_TOOLS)->addAction(cmd, Core::Constants::G_DEFAULT_THREE);

    return true;
}

ExtensionSystem::IPlugin::ShutdownFlag DoNothingPlugin::shutdown()
{
    return SynchronousShutdown;
}

Q_EXPORT_PLUGIN(DoNothingPlugin)
