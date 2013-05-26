    LogOutput,
    Constants::LOGAPP},
    Constants::ANNOTATEAPP},
    Constants::DIFFAPP}
    if (raiseSubmitEditor())
    setSubmitEditor(commitEditor);
bool MercurialPlugin::submitEditorAboutToClose()
    CommitEditor *commitEditor = qobject_cast<CommitEditor *>(submitEditor());
    QTC_ASSERT(commitEditor, return true);
    Core::IDocument *editorFile = commitEditor->document();
    QTC_ASSERT(editorFile, return true);
    MercurialEditor editor(editorParameters + 2, 0);
    MercurialEditor editor(editorParameters, 0);