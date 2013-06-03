    RegularCommandOutput, //type
    Constants::COMMANDLOG_ID, // id
    Constants::COMMANDLOG_DISPLAY_NAME, // display name
    Constants::COMMANDLOG, // context
    Constants::COMMANDAPP, // mime type
    Constants::COMMANDEXT}, //extension

{   LogOutput,
    Constants::LOGAPP,
    Constants::LOGEXT},
    Constants::ANNOTATEAPP,
    Constants::ANNOTATEEXT},
    Constants::DIFFAPP,
    Constants::DIFFEXT}
        VcsBasePlugin(QLatin1String(Constants::COMMIT_ID)),
    if (VcsBaseSubmitEditor::raiseSubmitEditor())
bool MercurialPlugin::submitEditorAboutToClose(VcsBaseSubmitEditor *submitEditor)
    Core::IDocument *editorFile = submitEditor->document();
    CommitEditor *commitEditor = qobject_cast<CommitEditor *>(submitEditor);
    if (!editorFile || !commitEditor)
        return true;
    MercurialEditor editor(editorParameters + 3, 0);
    MercurialEditor editor(editorParameters + 1, 0);