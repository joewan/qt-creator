/**************************************************************************
**
** Copyright (c) 2014 Brian McGillion and Hugues Delorme
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

#include "vcsbaseclient.h"
#include "command.h"
#include "vcsbaseclientsettings.h"
#include "vcsbaseeditorparameterwidget.h"

#include <coreplugin/icore.h>
#include <coreplugin/vcsmanager.h>

#include <utils/qtcassert.h>
#include <utils/synchronousprocess.h>
#include <vcsbase/vcsbaseeditor.h>
#include <vcsbase/vcsbaseoutputwindow.h>
#include <vcsbase/vcsbaseplugin.h>

#include <QStringList>
#include <QDir>
#include <QProcess>
#include <QSignalMapper>
#include <QTextCodec>
#include <QDebug>
#include <QFileInfo>
#include <QByteArray>
#include <QVariant>
#include <QProcessEnvironment>

/*!
    \class VcsBase::VcsBaseClient

    \brief The VcsBaseClient class is the base class for Mercurial and Bazaar
    'clients'.

    Provides base functionality for common commands (diff, log, etc).

    \sa VcsBase::VcsJobRunner
*/

Q_DECLARE_METATYPE(QVariant)

inline Core::IEditor *locateEditor(const char *property, const QString &entry)
{
    foreach (Core::IDocument *document, Core::EditorManager::documentModel()->openedDocuments())
        if (document->property(property).toString() == entry)
            return Core::EditorManager::documentModel()->editorsForDocument(document).first();
    return 0;
}

namespace {

VcsBase::VcsBaseOutputWindow *vcsOutputWindow()
{
    return VcsBase::VcsBaseOutputWindow::instance();
}

}

namespace VcsBase {

class VcsBaseClientPrivate
{
public:
    VcsBaseClientPrivate(VcsBaseClient *client, VcsBaseClientSettings *settings);

    void statusParser(const QString &text);
    void annotateRevision(const QString &workingDirectory, const QString &file,
                          QString change, int lineNumber);
    void saveSettings();

    void bindCommandToEditor(Command *cmd, VcsBaseEditorWidget *editor);
    void commandFinishedGotoLine(QWidget *editorObject);

    VcsBaseClientSettings *m_clientSettings;
    QSignalMapper *m_cmdFinishedMapper;

private:
    VcsBaseClient *m_client;
};

VcsBaseClientPrivate::VcsBaseClientPrivate(VcsBaseClient *client, VcsBaseClientSettings *settings) :
    m_clientSettings(settings),
    m_cmdFinishedMapper(new QSignalMapper(client)),
    m_client(client)
{
}

void VcsBaseClientPrivate::statusParser(const QString &text)
{
    QList<VcsBaseClient::StatusItem> lineInfoList;

    QStringList rawStatusList = text.split(QLatin1Char('\n'));

    foreach (const QString &string, rawStatusList) {
        const VcsBaseClient::StatusItem lineInfo = m_client->parseStatusLine(string);
        if (!lineInfo.flags.isEmpty() && !lineInfo.file.isEmpty())
            lineInfoList.append(lineInfo);
    }

    emit m_client->parsedStatus(lineInfoList);
}

void VcsBaseClientPrivate::annotateRevision(const QString &workingDirectory,  const QString &file,
                                            QString change, int lineNumber)
{
    // This might be invoked with a verbose revision description
    // "SHA1 author subject" from the annotation context menu. Strip the rest.
    const int blankPos = change.indexOf(QLatin1Char(' '));
    if (blankPos != -1)
        change.truncate(blankPos);
    m_client->annotate(workingDirectory, file, change, lineNumber);
}

void VcsBaseClientPrivate::saveSettings()
{
    m_clientSettings->writeSettings(Core::ICore::settings());
}

void VcsBaseClientPrivate::bindCommandToEditor(Command *cmd, VcsBaseEditorWidget *editor)
{
    editor->setCommand(cmd);
    QObject::connect(cmd, SIGNAL(finished(bool,int,QVariant)), m_cmdFinishedMapper, SLOT(map()));
    m_cmdFinishedMapper->setMapping(cmd, editor);
}

void VcsBaseClientPrivate::commandFinishedGotoLine(QWidget *editorObject)
{
    VcsBase::VcsBaseEditorWidget *editor = qobject_cast<VcsBase::VcsBaseEditorWidget *>(editorObject);
    Command *cmd = qobject_cast<Command *>(m_cmdFinishedMapper->mapping(editor));
    if (editor && cmd) {
        if (!cmd->lastExecutionSuccess()) {
            editor->reportCommandFinished(false, cmd->lastExecutionExitCode(), cmd->cookie());
        } else if (cmd->cookie().type() == QVariant::Int) {
            const int line = cmd->cookie().toInt();
            if (line >= 0)
                editor->gotoLine(line);
        }
        m_cmdFinishedMapper->removeMappings(cmd);
    }
}

VcsBaseClient::StatusItem::StatusItem(const QString &s, const QString &f) :
    flags(s), file(f)
{
}

VcsBaseClient::VcsBaseClient(VcsBaseClientSettings *settings) :
    d(new VcsBaseClientPrivate(this, settings))
{
    qRegisterMetaType<QVariant>();
    connect(Core::ICore::instance(), SIGNAL(saveSettingsRequested()), this, SLOT(saveSettings()));
    connect(d->m_cmdFinishedMapper, SIGNAL(mapped(QWidget*)), this, SLOT(commandFinishedGotoLine(QWidget*)));
}

VcsBaseClient::~VcsBaseClient()
{
    delete d;
}

bool VcsBaseClient::synchronousCreateRepository(const QString &workingDirectory,
                                                const QStringList &extraOptions)
{
    QStringList args(vcsCommandString(CreateRepositoryCommand));
    args << extraOptions;
    QByteArray outputData;
    if (!vcsFullySynchronousExec(workingDirectory, args, &outputData))
        return false;
    ::vcsOutputWindow()->append(
                Utils::SynchronousProcess::normalizeNewlines(QString::fromLocal8Bit(outputData)));

    resetCachedVcsInfo(workingDirectory);

    return true;
}

bool VcsBaseClient::synchronousClone(const QString &workingDir,
                                     const QString &srcLocation,
                                     const QString &dstLocation,
                                     const QStringList &extraOptions)
{
    QStringList args;
    args << vcsCommandString(CloneCommand)
         << extraOptions << srcLocation << dstLocation;
    QByteArray stdOut;
    const bool cloneOk = vcsFullySynchronousExec(workingDir, args, &stdOut);
    resetCachedVcsInfo(workingDir);
    return cloneOk;
}

bool VcsBaseClient::synchronousAdd(const QString &workingDir, const QString &filename,
                                   const QStringList &extraOptions)
{
    QStringList args;
    args << vcsCommandString(AddCommand) << extraOptions << filename;
    QByteArray stdOut;
    return vcsFullySynchronousExec(workingDir, args, &stdOut);
}

bool VcsBaseClient::synchronousRemove(const QString &workingDir, const QString &filename,
                                      const QStringList &extraOptions)
{
    QStringList args;
    args << vcsCommandString(RemoveCommand) << extraOptions << filename;
    QByteArray stdOut;
    return vcsFullySynchronousExec(workingDir, args, &stdOut);
}

bool VcsBaseClient::synchronousMove(const QString &workingDir,
                                    const QString &from, const QString &to,
                                    const QStringList &extraOptions)
{
    QStringList args;
    args << vcsCommandString(MoveCommand) << extraOptions << from << to;
    QByteArray stdOut;
    return vcsFullySynchronousExec(workingDir, args, &stdOut);
}

bool VcsBaseClient::synchronousPull(const QString &workingDir,
                                    const QString &srcLocation,
                                    const QStringList &extraOptions)
{
    QStringList args;
    args << vcsCommandString(PullCommand) << extraOptions << srcLocation;
    // Disable UNIX terminals to suppress SSH prompting
    const unsigned flags =
            VcsBase::VcsBasePlugin::SshPasswordPrompt
            | VcsBase::VcsBasePlugin::ShowStdOutInLogWindow
            | VcsBase::VcsBasePlugin::ShowSuccessMessage;
    const Utils::SynchronousProcessResponse resp = vcsSynchronousExec(workingDir, args, flags);
    const bool ok = resp.result == Utils::SynchronousProcessResponse::Finished;
    if (ok)
        emit changed(QVariant(workingDir));
    return ok;
}

bool VcsBaseClient::synchronousPush(const QString &workingDir,
                                    const QString &dstLocation,
                                    const QStringList &extraOptions)
{
    QStringList args;
    args << vcsCommandString(PushCommand) << extraOptions << dstLocation;
    // Disable UNIX terminals to suppress SSH prompting
    const unsigned flags =
            VcsBase::VcsBasePlugin::SshPasswordPrompt
            | VcsBase::VcsBasePlugin::ShowStdOutInLogWindow
            | VcsBase::VcsBasePlugin::ShowSuccessMessage;
    const Utils::SynchronousProcessResponse resp = vcsSynchronousExec(workingDir, args, flags);
    return resp.result == Utils::SynchronousProcessResponse::Finished;
}

bool VcsBaseClient::vcsFullySynchronousExec(const QString &workingDir,
                                            const QStringList &args,
                                            QByteArray *output) const
{
    QProcess vcsProcess;
    if (!workingDir.isEmpty())
        vcsProcess.setWorkingDirectory(workingDir);
    vcsProcess.setProcessEnvironment(processEnvironment());

    const QString binary = settings()->binaryPath();

    ::vcsOutputWindow()->appendCommand(workingDir, binary, args);

    vcsProcess.start(binary, args);

    if (!vcsProcess.waitForStarted()) {
        ::vcsOutputWindow()->appendError(tr("Unable to start process \"%1\": %2")
                                         .arg(QDir::toNativeSeparators(binary), vcsProcess.errorString()));
        return false;
    }

    vcsProcess.closeWriteChannel();

    QByteArray stdErr;
    const int timeoutSec = settings()->intValue(VcsBaseClientSettings::timeoutKey);
    if (!Utils::SynchronousProcess::readDataFromProcess(vcsProcess, timeoutSec * 1000,
                                                        output, &stdErr, true)) {
        Utils::SynchronousProcess::stopProcess(vcsProcess);
        ::vcsOutputWindow()->appendError(tr("Timed out after %1s waiting for the process %2 to finish.")
                                         .arg(timeoutSec).arg(binary));
        return false;
    }
    if (!stdErr.isEmpty())
        ::vcsOutputWindow()->appendError(QString::fromLocal8Bit(stdErr));

    return vcsProcess.exitStatus() == QProcess::NormalExit && vcsProcess.exitCode() == 0;
}

Utils::SynchronousProcessResponse VcsBaseClient::vcsSynchronousExec(
        const QString &workingDirectory,
        const QStringList &args,
        unsigned flags,
        QTextCodec *outputCodec)
{
    const QString binary = settings()->binaryPath();
    const int timeoutSec = settings()->intValue(VcsBaseClientSettings::timeoutKey);
    return VcsBase::VcsBasePlugin::runVcs(workingDirectory, binary, args, timeoutSec * 1000,
                                          flags, outputCodec);
}

void VcsBaseClient::annotate(const QString &workingDir, const QString &file,
                             const QString &revision /* = QString() */,
                             int lineNumber /* = -1 */,
                             const QStringList &extraOptions)
{
    const QString vcsCmdString = vcsCommandString(AnnotateCommand);
    QStringList args;
    args << vcsCmdString << revisionSpec(revision) << extraOptions << file;
    const Core::Id kind = vcsEditorKind(AnnotateCommand);
    const QString id = VcsBase::VcsBaseEditorWidget::getSource(workingDir, QStringList(file));
    const QString title = vcsEditorTitle(vcsCmdString, id);
    const QString source = VcsBase::VcsBaseEditorWidget::getSource(workingDir, file);

    VcsBase::VcsBaseEditorWidget *editor = createVcsEditor(kind, title, source, true,
                                                           vcsCmdString.toLatin1().constData(), id);

    Command *cmd = createCommand(workingDir, editor);
    cmd->setCookie(lineNumber);
    enqueueJob(cmd, args);
}

void VcsBaseClient::diff(const QString &workingDir, const QStringList &files,
                         const QStringList &extraOptions)
{
    const QString vcsCmdString = vcsCommandString(DiffCommand);
    const Core::Id kind = vcsEditorKind(DiffCommand);
    const QString id = VcsBase::VcsBaseEditorWidget::getTitleId(workingDir, files);
    const QString title = vcsEditorTitle(vcsCmdString, id);
    const QString source = VcsBase::VcsBaseEditorWidget::getSource(workingDir, files);
    VcsBase::VcsBaseEditorWidget *editor = createVcsEditor(kind, title, source, true,
                                                           vcsCmdString.toLatin1().constData(), id);
    editor->setWorkingDirectory(workingDir);

    VcsBaseEditorParameterWidget *paramWidget = editor->configurationWidget();
    if (!paramWidget && (paramWidget = createDiffEditor(workingDir, files, extraOptions))) {
        // editor has been just created, createVcsEditor() didn't set a configuration widget yet
        connect(editor, SIGNAL(diffChunkReverted(VcsBase::DiffChunk)), paramWidget, SLOT(executeCommand()));
        editor->setConfigurationWidget(paramWidget);
    }

    QStringList args;
    const QStringList paramArgs = paramWidget != 0 ? paramWidget->arguments() : QStringList();
    args << vcsCmdString << extraOptions << paramArgs << files;
    QTextCodec *codec = source.isEmpty() ? static_cast<QTextCodec *>(0) : VcsBase::VcsBaseEditorWidget::getCodec(source);
    Command *command = createCommand(workingDir, editor);
    command->setCodec(codec);
    enqueueJob(command, args, exitCodeInterpreter(DiffCommand, command));
}

void VcsBaseClient::log(const QString &workingDir, const QStringList &files,
                        const QStringList &extraOptions,
                        bool enableAnnotationContextMenu)
{
    const QString vcsCmdString = vcsCommandString(LogCommand);
    const Core::Id kind = vcsEditorKind(LogCommand);
    const QString id = VcsBase::VcsBaseEditorWidget::getTitleId(workingDir, files);
    const QString title = vcsEditorTitle(vcsCmdString, id);
    const QString source = VcsBase::VcsBaseEditorWidget::getSource(workingDir, files);
    VcsBase::VcsBaseEditorWidget *editor = createVcsEditor(kind, title, source, true,
                                                           vcsCmdString.toLatin1().constData(), id);
    editor->setFileLogAnnotateEnabled(enableAnnotationContextMenu);

    VcsBaseEditorParameterWidget *paramWidget = editor->configurationWidget();
    if (!paramWidget && (paramWidget = createLogEditor(workingDir, files, extraOptions))) {
        // editor has been just created, createVcsEditor() didn't set a configuration widget yet
        editor->setConfigurationWidget(paramWidget);
    }

    QStringList args;
    const QStringList paramArgs = paramWidget != 0 ? paramWidget->arguments() : QStringList();
    args << vcsCmdString << extraOptions << paramArgs << files;
    enqueueJob(createCommand(workingDir, editor), args);
}

void VcsBaseClient::revertFile(const QString &workingDir,
                               const QString &file,
                               const QString &revision,
                               const QStringList &extraOptions)
{
    QStringList args(vcsCommandString(RevertCommand));
    args << revisionSpec(revision) << extraOptions << file;
    // Indicate repository change or file list
    Command *cmd = createCommand(workingDir);
    cmd->setCookie(QStringList(workingDir + QLatin1Char('/') + file));
    connect(cmd, SIGNAL(success(QVariant)), this, SIGNAL(changed(QVariant)), Qt::QueuedConnection);
    enqueueJob(cmd, args);
}

void VcsBaseClient::revertAll(const QString &workingDir, const QString &revision,
                              const QStringList &extraOptions)
{
    QStringList args(vcsCommandString(RevertCommand));
    args << revisionSpec(revision) << extraOptions;
    // Indicate repository change or file list
    Command *cmd = createCommand(workingDir);
    cmd->setCookie(QStringList(workingDir));
    connect(cmd, SIGNAL(success(QVariant)), this, SIGNAL(changed(QVariant)), Qt::QueuedConnection);
    enqueueJob(createCommand(workingDir), args);
}

void VcsBaseClient::status(const QString &workingDir, const QString &file,
                           const QStringList &extraOptions)
{
    QStringList args(vcsCommandString(StatusCommand));
    args << extraOptions << file;
    ::vcsOutputWindow()->setRepository(workingDir);
    Command *cmd = createCommand(workingDir, 0, VcsWindowOutputBind);
    connect(cmd, SIGNAL(finished(bool,int,QVariant)), ::vcsOutputWindow(), SLOT(clearRepository()),
            Qt::QueuedConnection);
    enqueueJob(cmd, args);
}

void VcsBaseClient::emitParsedStatus(const QString &repository, const QStringList &extraOptions)
{
    QStringList args(vcsCommandString(StatusCommand));
    args << extraOptions;
    Command *cmd = createCommand(repository);
    connect(cmd, SIGNAL(output(QString)), this, SLOT(statusParser(QString)));
    enqueueJob(cmd, args);
}

QString VcsBaseClient::vcsCommandString(VcsCommand cmd) const
{
    switch (cmd) {
    case CreateRepositoryCommand: return QLatin1String("init");
    case CloneCommand: return QLatin1String("clone");
    case AddCommand: return QLatin1String("add");
    case RemoveCommand: return QLatin1String("remove");
    case MoveCommand: return QLatin1String("rename");
    case PullCommand: return QLatin1String("pull");
    case PushCommand: return QLatin1String("push");
    case CommitCommand: return QLatin1String("commit");
    case ImportCommand: return QLatin1String("import");
    case UpdateCommand: return QLatin1String("update");
    case RevertCommand: return QLatin1String("revert");
    case AnnotateCommand: return QLatin1String("annotate");
    case DiffCommand: return QLatin1String("diff");
    case LogCommand: return QLatin1String("log");
    case StatusCommand: return QLatin1String("status");
    }
    return QString();
}

Utils::ExitCodeInterpreter *VcsBaseClient::exitCodeInterpreter(VcsCommand cmd, QObject *parent) const
{
    Q_UNUSED(cmd)
    Q_UNUSED(parent)
    return 0;
}

void VcsBaseClient::import(const QString &repositoryRoot, const QStringList &files,
                           const QStringList &extraOptions)
{
    QStringList args(vcsCommandString(ImportCommand));
    args << extraOptions << files;
    enqueueJob(createCommand(repositoryRoot), args);
}

void VcsBaseClient::view(const QString &source, const QString &id,
                         const QStringList &extraOptions)
{
    QStringList args;
    args << extraOptions << revisionSpec(id);
    const Core::Id kind = vcsEditorKind(DiffCommand);
    const QString title = vcsEditorTitle(vcsCommandString(LogCommand), id);

    VcsBase::VcsBaseEditorWidget *editor = createVcsEditor(kind, title, source,
                                                           true, "view", id);

    const QFileInfo fi(source);
    const QString workingDirPath = fi.isFile() ? fi.absolutePath() : source;
    enqueueJob(createCommand(workingDirPath, editor), args);
}

void VcsBaseClient::update(const QString &repositoryRoot, const QString &revision,
                           const QStringList &extraOptions)
{
    QStringList args(vcsCommandString(UpdateCommand));
    args << revisionSpec(revision) << extraOptions;
    Command *cmd = createCommand(repositoryRoot);
    cmd->setCookie(repositoryRoot);
    connect(cmd, SIGNAL(success(QVariant)), this, SIGNAL(changed(QVariant)), Qt::QueuedConnection);
    enqueueJob(cmd, args);
}

void VcsBaseClient::commit(const QString &repositoryRoot,
                           const QStringList &files,
                           const QString &commitMessageFile,
                           const QStringList &extraOptions)
{
    // Handling of commitMessageFile is a bit tricky :
    //   VcsBaseClient cannot do something with it because it doesn't know which
    //   option to use (-F ? but sub VCS clients might require a different option
    //   name like -l for hg ...)
    //
    //   So descendants of VcsBaseClient *must* redefine commit() and extend
    //   extraOptions with the usage for commitMessageFile (see BazaarClient::commit()
    //   for example)
    Q_UNUSED(commitMessageFile);
    QStringList args(vcsCommandString(CommitCommand));
    args << extraOptions << files;
    enqueueJob(createCommand(repositoryRoot), args);
}

VcsBaseClientSettings *VcsBaseClient::settings() const
{
    return d->m_clientSettings;
}

VcsBaseEditorParameterWidget *VcsBaseClient::createDiffEditor(const QString &workingDir,
                                                              const QStringList &files,
                                                              const QStringList &extraOptions)
{
    Q_UNUSED(workingDir);
    Q_UNUSED(files);
    Q_UNUSED(extraOptions);
    return 0;
}

VcsBaseEditorParameterWidget *VcsBaseClient::createLogEditor(const QString &workingDir,
                                                             const QStringList &files,
                                                             const QStringList &extraOptions)
{
    Q_UNUSED(workingDir);
    Q_UNUSED(files);
    Q_UNUSED(extraOptions);
    return 0;
}

QString VcsBaseClient::vcsEditorTitle(const QString &vcsCmd, const QString &sourceId) const
{
    const QString binary = settings()->binaryPath();
    return QFileInfo(binary).baseName() +
            QLatin1Char(' ') + vcsCmd + QLatin1Char(' ') +
            QFileInfo(sourceId).fileName();
}

VcsBase::VcsBaseEditorWidget *VcsBaseClient::createVcsEditor(Core::Id kind, QString title,
                                                             const QString &source, bool setSourceCodec,
                                                             const char *registerDynamicProperty,
                                                             const QString &dynamicPropertyValue) const
{
    VcsBase::VcsBaseEditorWidget *baseEditor = 0;
    Core::IEditor *outputEditor = locateEditor(registerDynamicProperty, dynamicPropertyValue);
    const QString progressMsg = tr("Working...");
    if (outputEditor) {
        // Exists already
        outputEditor->document()->setContents(progressMsg.toUtf8());
        baseEditor = VcsBase::VcsBaseEditorWidget::getVcsBaseEditor(outputEditor);
        QTC_ASSERT(baseEditor, return 0);
        Core::EditorManager::activateEditor(outputEditor);
    } else {
        outputEditor
                = Core::EditorManager::openEditorWithContents(kind, &title, progressMsg.toUtf8(),
                                                              (Core::EditorManager::OpenInOtherSplit
                                                               | Core::EditorManager::NoNewSplits));
        outputEditor->document()->setProperty(registerDynamicProperty, dynamicPropertyValue);
        baseEditor = VcsBase::VcsBaseEditorWidget::getVcsBaseEditor(outputEditor);
        connect(baseEditor, SIGNAL(annotateRevisionRequested(QString,QString,QString,int)),
                this, SLOT(annotateRevision(QString,QString,QString,int)));
        QTC_ASSERT(baseEditor, return 0);
        baseEditor->setSource(source);
        if (setSourceCodec)
            baseEditor->setCodec(VcsBase::VcsBaseEditorWidget::getCodec(source));
    }

    baseEditor->setForceReadOnly(true);
    return baseEditor;
}

QProcessEnvironment VcsBaseClient::processEnvironment() const
{
    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    VcsBase::VcsBasePlugin::setProcessEnvironment(&environment, false);
    return environment;
}

Command *VcsBaseClient::createCommand(const QString &workingDirectory,
                                      VcsBase::VcsBaseEditorWidget *editor,
                                      JobOutputBindMode mode)
{
    Command *cmd = new Command(d->m_clientSettings->binaryPath(),
                               workingDirectory, processEnvironment());
    cmd->setDefaultTimeout(d->m_clientSettings->intValue(VcsBaseClientSettings::timeoutKey));
    if (editor)
        d->bindCommandToEditor(cmd, editor);
    if (mode == VcsWindowOutputBind) {
        cmd->addFlags(VcsBasePlugin::ShowStdOutInLogWindow);
        if (editor) // assume that the commands output is the important thing
            cmd->addFlags(VcsBasePlugin::SilentOutput);
    } else if (editor) {
        connect(cmd, SIGNAL(output(QString)), editor, SLOT(setPlainText(QString)));
    }

    return cmd;
}

void VcsBaseClient::enqueueJob(Command *cmd, const QStringList &args, Utils::ExitCodeInterpreter *interpreter)
{
    cmd->addJob(args, interpreter);
    cmd->execute();
}

void VcsBaseClient::resetCachedVcsInfo(const QString &workingDir)
{
    Core::VcsManager::resetVersionControlForDirectory(workingDir);
}

} // namespace VcsBase

#include "moc_vcsbaseclient.cpp"
