source("../../shared/qtcreator.py")
source("../../shared/suites_qtta.py")

webPageContentLoadedValue = 0
gettingStartedText = getQmlItem("Text", ":Qt Creator_QDeclarativeView", False,
                                "text='Getting Started'")

# wait until help gets loaded
def webPageContentLoaded(obj, param):
    global webPageContentLoadedValue
    objectClass = str(obj.metaObject().className())
    if objectClass in ("QWebPage", "Help::Internal::HelpViewer"):
        webPageContentLoadedValue += 1

def clickItemVerifyHelpCombo(qmlItem, expectedHelpComboText, testDetails):
    global gettingStartedText
    webPageContentLoadedValue = 0
    mouseClick(waitForObject(qmlItem), 5, 5, 0, Qt.LeftButton)
    waitFor("webPageContentLoadedValue == 4", 5000)
    test.compare(waitForObject(":Qt Creator_HelpSelector_QComboBox").currentText,
                 expectedHelpComboText, testDetails)
    # select "Welcome" page from left toolbar again
    switchViewTo(ViewConstants.WELCOME)
    test.verify(checkIfObjectExists(gettingStartedText),
                "Verifying: Getting Started topic is being displayed.")

def main():
    global webPageContentLoadedValue, gettingStartedText
    # open Qt Creator
    startApplication("qtcreator" + SettingsPath)
    installLazySignalHandler(":QWebPage","loadFinished(bool)", "webPageContentLoaded")
    installLazySignalHandler(":*Qt Creator_Help::Internal::HelpViewer", "loadFinished(bool)",
                             "webPageContentLoaded")
    setAlwaysStartFullHelp()
    if not test.verify(checkIfObjectExists(gettingStartedText),
                       "Verifying: Qt Creator displays Welcome Page with Getting Started."):
        mouseClick(waitForObject(getQmlItem("LinkedText", ":Qt Creator_QDeclarativeView", False,
                                            "text='Getting Started'")), 5, 5, 0, Qt.LeftButton)
    qmlItem = getQmlItem("LinkedText", ":Qt Creator_QDeclarativeView", False, "text='User Guide'")
    expectedText = "QtCreator : Qt Creator Manual"
    testDetails = "Verifying: Help with Creator Documentation is being opened."
    # select "User Guide" topic
    clickItemVerifyHelpCombo(qmlItem, expectedText, testDetails)
    # check "Online Community" link
    test.verify(checkIfObjectExists(getQmlItem("LinkedText", ":Qt Creator_QDeclarativeView", False,
                                               "text='Online Community'")),
                "Verifying: Link to Qt forums exists.")
    test.verify(checkIfObjectExists(getQmlItem("LinkedText", ":Qt Creator_QDeclarativeView", False,
                                               "text='Blogs'")),
                "Verifying: Link to Planet Qt exists.")
    qmlItem = getQmlItem("Text", ":Qt Creator_QDeclarativeView", False, "text='IDE Overview'")
    expectedText = "QtCreator : IDE Overview"
    testDetails = "Verifying: Help with IDE Overview topic is being opened."
    # select "IDE Overview"
    clickItemVerifyHelpCombo(qmlItem, expectedText, testDetails)
    qmlItem = getQmlItem("Text", ":Qt Creator_QDeclarativeView", False, "text='User Interface'")
    expectedText = "QtCreator : User Interface"
    testDetails = "Verifying: Help with User Interface topic is being opened."
    # select "User interface"
    clickItemVerifyHelpCombo(qmlItem, expectedText, testDetails)
    # select "Building and Running an Example Application"
    webPageContentLoadedValue = 0
    mouseClick(waitForObject(getQmlItem("Text", ":Qt Creator_QDeclarativeView", False,
                                        "text='Building and Running an Example Application'")),
                                        5, 5, 0, Qt.LeftButton)
    waitFor("webPageContentLoadedValue == 4", 5000)
    checkPattern = "QtCreator : Building and Running an Example( Application)?"
    checkText = str(waitForObject(":Qt Creator_HelpSelector_QComboBox").currentText)
    if not test.verify(re.search(checkPattern, checkText),
                       "Verifying: Building and Running an Example is opened."):
        test.fail("Pattern does not match: '%s', text found in QComboBox is: "
                  "'%s'" % (checkPattern, checkText))
    # select "Welcome" page from left toolbar again
    switchViewTo(ViewConstants.WELCOME)
    test.verify(checkIfObjectExists(gettingStartedText),
                "Verifying: Getting Started topic is being displayed.")
    # select "Start Developing"
    mouseClick(waitForObject(getQmlItem("Text", ":Qt Creator_QDeclarativeView", False,
                                        "text='Start Developing'")), 5, 5, 0, Qt.LeftButton)
    test.verify(checkIfObjectExists(getQmlItem("Text", ":Qt Creator_QDeclarativeView", False,
                                               "text='Tutorials'")),
                "Verifying: Tutorials are opened in Welcome Page.")
    # exit Qt Creator
    invokeMenuItem("File", "Exit")
