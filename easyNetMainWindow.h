#ifndef NMCONSOLE_H
#define NMCONSOLE_H

#include <QMainWindow>
#include <QLineEdit>
#include <QPlainTextEdit>


QT_BEGIN_NAMESPACE
class QLabel;
class QWebView;
class QGroupBox;
class QDockWidget;
class QSignalMapper;

class ObjExplorer;

class DesignWindow;
class LazyNut;
class CommandSequencer;
class SessionManager;
class LazyNutObject;
typedef QHash<QString,LazyNutObject*> LazyNutObjectCatalogue;
class EditWindow;
class Highlighter;
class PlotWindow;


QT_END_NAMESPACE

class InputCmdLine : public QLineEdit
{
    Q_OBJECT

public:
    InputCmdLine(QWidget *parent = 0);

public slots:
    void sendCommand();

signals:
    void commandReady(const QString & command);

};

class CmdOutput : public QPlainTextEdit
{
    Q_OBJECT

public:
    CmdOutput(QWidget *parent = 0);

public slots:
    void displayOutput(const QString & output);

};

class LazyNutScriptEditor: public QPlainTextEdit
{
    Q_OBJECT

public:
    LazyNutScriptEditor(QWidget *parent = 0);

public slots:
    QStringList getSelectedText();
    QStringList getAllText();

};


class EasyNetMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    enum ViewMode
    {
        ViewMode_BEGIN,
        Welcome = ViewMode_BEGIN,
        Model,
        Trial,
        Input,
        Output,
        Params,
        Interpreter,
        Code,
        ViewMode_END
    };
    EasyNetMainWindow(QWidget *parent = 0);

signals:
    void savedLayoutToBeLoaded(QString);
    void saveLayout();
    void viewModeClicked(int);

private slots:
    void showViewMode(int viewModeInt);
    void newScriptFile();
    void newLogFile();
    void open();
    void run();
    //bool save();
    //bool saveAs();
    //void documentWasModified();

    void echoCommand(const QString &line);
//    void runScript();
    void runModel();
    void runSelection();
    void runCmd(QString cmd);
    void setEasyNetHome();
    void setLazyNutBat();
    void showPauseState(bool isPaused);

    void lazyNutNotRunning();
    void displayVersion(QString version);

protected:
    void closeEvent(QCloseEvent *event);

private:
    void initViewActions();
    void createViewActions();
    void createActions();
    void createMenus();
    void createToolBars();
//    bool maybeSave();
    void newFile(EditWindow*);
    void loadFile(const QString &fileName);
//    bool saveFile(const QString &fileName);

    void setCurrentFile(EditWindow *window, const QString &fileName);
    QString strippedName(const QString &fullFileName);
    void readSettings();
    void writeSettings();
//    void runLazyNutBat();

    void hideAllDocks();


    QString         lazyNutBat= "";
//    QString         curFile;
#if defined(__linux__)
    QString         lazyNutExt = "sh";
    QString         binDir = "bin-linux";
#elif defined(__APPLE__)
    QString         lazyNutExt = "sh";
    QString         binDir = "bin-mac";
#elif defined(_WIN32)
    QString         lazyNutExt = "bat";
    QString         binDir = "bin";
#endif
    QString         lazyNutBasename = QString("lazyNut.%1").arg(lazyNutExt);
    QString         curFile;
    QString         curJson;
    QString         scriptsDir;
    QString         easyNetHome = "";


    QGroupBox       *lazyNutInterpreter;
    CmdOutput       *cmdOutput;
    InputCmdLine    *inputCmdLine;
    SessionManager      *sessionManager;
//    TreeModel       *objTaxonomyModel;
    LazyNutObjectCatalogue *objectCatalogue;
//    LazyNutObjCatalogue  *objCatalogue;
    ObjExplorer      *objExplorer;
    QLabel          *zebPic;

    QDockWidget     *dockZeb;
    QDockWidget     *dockWelcome;
    QDockWidget     *dockWebWelcome;
    QDockWidget     *dockEdit;
    QDockWidget     *dockCommandLog;
    QDockWidget     *dockInterpreter;
    QDockWidget     *dockExplorer;
    QDockWidget     *dockDesignWindow;
    QDockWidget     *dockInput;
    QDockWidget     *dockOutput;

    QWebView        *welcomeScreen;
    QWebView        *webWelcomeScreen;
//    CodeEditor      *scriptEdit;
    EditWindow       *scriptEdit;
//    CodeEditor      *commandLog;
    EditWindow       *commandLog;
    Highlighter     *highlighter;
    Highlighter     *highlighter2;
//    LazyNutScriptEditor  *scriptEditor;
    DesignWindow    *designWindow;
    PlotWindow      *plotForm;
    QToolBar        *infoToolBar;
//    QVBoxLayout     *vLayout;

    QSignalMapper   *viewModeSignalMapper;
    QList<QToolButton*> viewModeButtons;
    QMenu           *fileMenu;
    QMenu           *runMenu;
    QMenu           *settingsMenu;
    QMenu           *aboutMenu;
//    QToolBar        *fileToolBar;
//    QToolBar        *runToolBar;

    QList<QIcon*>   viewActionIcons;
    QStringList     viewActionTexts;
    QStringList     viewActionStatusTips;
    QList<QAction*> viewActions;

    QAction         *newScriptAct;
    QAction         *newLogAct;
    QAction         *openAct;
//    QAction         *saveAct;
//    QAction         *saveAsAct;
    QAction         *exitAct;
    QAction         *runAction;
    QAction         *runScriptAct;
    QAction         *runSelectionAct;
    QAction         *stopAct;
    QAction         *pauseAct;
    QAction         *setEasyNetHomeAct;
    QAction         *setLazyNutBatAct;
    QAction         *versionAct;
};

#endif // NMCONSOLE_H
