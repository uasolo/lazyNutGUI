#ifndef TRIALWIDGET_H
#define TRIALWIDGET_H

#include <QWidget>
#include <QMap>
#include <QSet>
#include <QSharedPointer>


class QComboBox;
class QLabel;
class QHBoxLayout;
class QVBoxLayout;
class ObjectCacheFilter;
class ObjectUpdater;
class QDomDocument;
class QAction;
class QToolButton;
class LazyNutJob;
class QMessageBox;
class QCheckBox;

class TrialWidget : public QWidget
{
    Q_OBJECT

public:
    TrialWidget(QWidget *parent=0);

    ~TrialWidget();

    QString getTrialCmd();
    QString getStochasticCmd();
    bool checkIfReadyToRun();
    QString getStimulusSet();
    QString defaultDataframe();
public slots:
    QStringList getArguments();
signals:
//    void trialDescriptionUpdated(QDomDocument*);
    void aboutToRunTrial(QSharedPointer<QDomDocument>);
    void trialRunModeChanged(int);

private slots:
    void runTrial();
    void update(QString trialName);
    void buildComboBoxes(QDomDocument* domDoc);
    void execBuildComboBoxes(QStringList args = QStringList());
    void setRunButtonIcon();
    void hideSetComboBox();
    void showSetComboBox();
    void showSetLabel(QString set);
    void clearArgumentBoxes();
    void clearDollarArgumentBoxes();
    void insertArgumentsInBoxes();
    void updateModelStochasticity(QDomDocument* modelDescription);
    void addParamExploreDf(QString name);
    void initParamExplore(QString name);
    void runParamExplore(QDomDocument *df, QString name);
    void setTrialRunMode(int mode);

private:
    void buildWidget();
    QSharedPointer<QDomDocument> createTrialRunInfo();
    void clearLayout(QLayout *layout);
    void runSingleTrial(LazyNutJob *job);
    void runTrialList(LazyNutJob *job);
    void setStochasticityVisible(bool isVisible);
    bool hasDollarArguments();

    ObjectCacheFilter* trialFilter;
    ObjectUpdater* trialDescriptionUpdater;
    ObjectCacheFilter* modelFilter;
    ObjectUpdater* modelDescriptionUpdater;
    ObjectCacheFilter *paramExploreFilter;
    ObjectUpdater* paramExploreDescriptionUpdater;
    ObjectCacheFilter *paramExploreDataframeFilter;
    ObjectUpdater* paramExploreDataframeUpdater;
    ObjectCacheFilter* setFilter;
    QString currentParamExplore;

    QMap <QString, QLabel*> labelMap;
    QMap <QString, QComboBox*> comboMap;
    QStringList argList;

    QComboBox*      setComboBox;
    QToolButton*    setCancelButton;
    QLabel*         repetitionsLabel;
    QComboBox*      repetitionsBox;
    QLabel*         strategyLabel;
    QComboBox*      strategyBox;

    QHBoxLayout*    layout1;
    QHBoxLayout*    layout2;
    QVBoxLayout*    layout3;
    QHBoxLayout*    layout;
    QAction*        runAction;
    QToolButton*    runButton;
    QAction*        hideSetComboBoxAction;
    QMessageBox *disableObserversMsg;
    QCheckBox *dontAskAgainDisableObserverCheckBox;
    bool        askDisableObserver;
    bool        suspendingObservers;

    QMap<QString,QString>     defs;
    int trialRunMode;
    bool isStochastic;

};

#endif // TRIALWIDGET_H
