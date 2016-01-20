#include "dataframeviewer.h"
#include "easyNetMainWindow.h"
#include "lazynutjob.h"
#include "enumclasses.h"
#include "sessionmanager.h"
#include "prettyheadersmodel.h"
#include "objectcachefilter.h"
#include "objectupdater.h"
#include "dataframemodel.h"
#include "ui_dataviewer.h"
#include "trialwidget.h"
#include "finddialog.h"




#include <QSettings>
#include <QFileDialog>
#include <QTableView>

DataframeViewer::DataframeViewer(Ui_DataViewer *ui, QWidget *parent)
    : DataViewer(ui, parent), m_dragDropColumns(false), m_stimulusSet(false)
{
    dataframeFilter = new ObjectCacheFilter(SessionManager::instance()->dataframeCache, this);
    dataframeUpdater = new ObjectUpdater(this);
    dataframeUpdater->setCommand("get");
    dataframeUpdater->setProxyModel(dataframeFilter);
    connect(dataframeUpdater, SIGNAL(objectUpdated(QDomDocument*,QString)),
            this, SLOT(updateDataframe(QDomDocument*,QString)));
    destroyedObjectsFilter->setType("dataframe");
    findDialog = new FindDialog(this);
    findDialog->hideExtendedOptions();
    connect(findDialog, SIGNAL(findForward(QString, QFlags<QTextDocument::FindFlag>)),
            this, SLOT(findForward(QString, QFlags<QTextDocument::FindFlag>)));
    ui->findAct->setVisible(true);
    connect(ui->findAct, SIGNAL(triggered()), this, SLOT(showFindDialog()));
    ui->findAct->setEnabled(false); // because DataframeViewer::enableActions won't be called by te ctor, only DataViewer::enableActions
}


bool DataframeViewer::contains(QString name)
{
    return modelMap.contains(name);
}

void DataframeViewer::open()
{

    QString fileName = QFileDialog::getOpenFileName(this,tr("Import dataframe"),
                                                    lastOpenDir.isEmpty() ? defaultOpenDir : lastOpenDir,
                                                    tr("Database Files (*.eNd);;Text files (*.csv);;All files (*.*)"));
    if (!fileName.isEmpty())
    {
        // create db
        QFileInfo fi(fileName);
        QString dfName = SessionManager::instance()->makeValidObjectName(fi.completeBaseName());
        lastOpenDir = fi.path();

        fileName = QDir(MainWindow::instance()->easyNetDataHome).relativeFilePath(fileName);
        QString loadCmd = fi.suffix() == "csv" ? "load_csv" : "load";
        QString dataframeType = stimulusSet() ? "stimulus_set" : "dataframe";
        LazyNutJob *job = new LazyNutJob;
        job->logMode |= ECHO_INTERPRETER;
        job->cmdList = QStringList({
                            QString("create %1 %2").arg(dataframeType).arg(dfName),
                            QString("%1 %2 %3").arg(dfName).arg(loadCmd).arg(fileName)});
        QList<LazyNutJob*> jobs = QList<LazyNutJob*>()
                << job
                << SessionManager::instance()->recentlyCreatedJob()
                << SessionManager::instance()->recentlyModifiedJob();
        QMap<QString, QVariant> jobData;
        jobData.insert("dfName", dfName);
        jobData.insert("setCurrent", true);
        jobs.last()->data = jobData;
        jobs.last()->appendEndOfJobReceiver(this, SLOT(addItem()));
        SessionManager::instance()->submitJobs(jobs);
    }
}

void DataframeViewer::save()
{
    QString fileName = QFileDialog::getSaveFileName(this,
                        tr("Save as CSV file"),
                        lastSaveDir.isEmpty() ? defaultSaveDir : lastOpenDir,
                        "CSV (*.csv)");
    if (!fileName.isEmpty())
    {
        LazyNutJob *job = new LazyNutJob;
        job->logMode |= ECHO_INTERPRETER;
        job->cmdList = QStringList({QString("%1 save_csv %2").arg(ui->currentItem()).arg(fileName)});
        SessionManager::instance()->submitJobs(job);
        lastSaveDir = QFileInfo(fileName).path();
    }
}

void DataframeViewer::copy()
{
    // this is an illegal approach -- get R to copy the df to the clipboard
    LazyNutJob *job = new LazyNutJob;
    job->logMode |= ECHO_INTERPRETER;
    job->cmdList = QStringList({QString("R << write.table(eN[\"%1\"], \"clipboard\", sep=\"\t\", row.names=FALSE)")
                                .arg(ui->currentItem())});
    SessionManager::instance()->submitJobs(job);
}

void DataframeViewer::initiateRemoveItem(QString name)
{
    if (name.contains(QRegExp("[()]"))) // don't destroy default dataframes
    {
        eNwarning << QString("attempt to delete lazyNut system object %1").arg(name);
        // this should change, trigger desable default observer
    }
    else
    {
        SessionManager::instance()->destroyObject(name);
    }
}

void DataframeViewer::removeItem(QString name)
{
    if (sender())
        qDebug() << sender()->metaObject()->className();
    if (!modelMap.contains(name))
    {
        eNwarning << QString("attempt to delete non-existing dataframe %1").arg(name);
    }
//    else if (!modelMap.value(name))
//    {
//        eNerror << QString("dataframe %1 does not have a DataFrameModel").arg(name);
//    }
//    else if (!viewsMap.values(name).at(0)) // to be changed for splitters
//    {
//        eNerror << QString("dataframe %1 does not have a view").arg(name);
//    }
//    else if (name.contains(QRegExp("[()]"))) // don't destroy default dataframes
//    {
//        qDebug() << Q_FUNC_INFO <<  "WARNING: attempt to delete a default dataframe";
//        // this should change, trigger desable default observer
//    }
    else
    {
        ui->removeItem(name);
        delete modelMap.value(name);
        modelMap.remove(name);
        if (viewsMap.values(name).count() > 0)
            delete viewsMap.values(name).at(0);
        viewsMap.remove(name);
        if (prettyHeadersModelMap.contains(name))
            delete prettyHeadersModelMap.value(name);
        if (!isLazy())
            dataframeFilter->removeName(name);
//        if (SessionManager::instance()->exists(name))
//        {
//            // delete lazyNut object
//            LazyNutJob *job = new LazyNutJob;
//            job->logMode |= ECHO_INTERPRETER;
//            job->cmdList = QStringList({QString("destroy %1").arg(name)});
//            QList<LazyNutJob*> jobs = QList<LazyNutJob*>()
//                    << job
//                    << SessionManager::instance()->recentlyDestroyedJob();
//            SessionManager::instance()->submitJobs(jobs);
//        }
    }
}

void DataframeViewer::enableActions(bool enable)
{
    DataViewer::enableActions(enable);
    ui->findAct->setEnabled(enable);
}

void DataframeViewer::updateCurrentItem(QString name)
{
    DataViewer::updateCurrentItem(name);
    if (isLazy())
        dataframeFilter->setName(name);
}



void DataframeViewer::addItem(QString name, bool setCurrent)
{
    if (name.isEmpty())
    {
        QVariant v = SessionManager::instance()->getDataFromJob(sender(), "dfName");
        if (!v.canConvert<QString>())
        {
            eNerror << "cannot retrieve a valid string from dfName key in sender LazyNut job";
            return;
        }
        name = v.value<QString>();
        v = SessionManager::instance()->getDataFromJob(sender(), "setCurrent");
        if (v.canConvert<bool>())
            setCurrent = v.value<bool>();
    }
    if (name.isEmpty())
    {
        eNerror << "string from dfName key in sender LazyNut job is empty";
    }
    else if (!SessionManager::instance()->exists(name))
    {
        eNerror << QString("attempt to add a non-existing dataframe %1").arg(name);
    }
    else if (modelMap.contains(name))
    {
//        eNerror << QString("attempt to create an already existing model for dataframe %1").arg(name);
        if (setCurrent)
            ui->setCurrentItem(name);
    }
    else
    {
        modelMap.insert(name, nullptr);
        ui->addItem(name, new QWidget(ui));
        if (setCurrent)
            ui->setCurrentItem(name);
        if (!isLazy())
            dataframeFilter->addName(name);
    }
}

void DataframeViewer::updateDataframe(QDomDocument *domDoc, QString name)
{
    if (!modelMap.contains(name))
    {
        eNerror << QString("attempt to update non-existing dataframe %1").arg(name);
    }
    DataFrameModel *dfModel = new DataFrameModel(domDoc, this);
    dfModel->setName(name); // needed?
    PrettyHeadersModel *prettyHeadersModel = nullptr;
    if (prettyHeadersModelMap.contains(name))
    {
        prettyHeadersModel = prettyHeadersModelMap.value(name);
        prettyHeadersModel->setSourceModel(dfModel);
    }
    bool isNewModel = (modelMap.value(name) == nullptr);
    // needs to be changed for splitters
    QTableView *view;
    if (isNewModel)
    {
        view = new QTableView(this);
        if (dragDropColumns())
        {
             DataFrameHeader* dragDropHeader = new DataFrameHeader(view);
             view->setHorizontalHeader(dragDropHeader);
             dragDropHeader->setTableName(name);
             connect(dragDropHeader, SIGNAL(columnDropped(QString)),
                     MainWindow::instance()->trialWidget, SLOT(showSetLabel(QString)));
             connect(dragDropHeader, SIGNAL(restoreComboBoxText()),
                     MainWindow::instance()->trialWidget, SLOT(restoreComboBoxText()));
        }
        viewsMap.insert(name, view);
        ui->addItem(name, view);
    }
    else
    {
        DataFrameModel *oldDFmodel = modelMap.value(name);
        view = viewsMap.values(name).at(0); // take the first and only one for the moment, to be changed for splitters
        QItemSelectionModel *m = view->selectionModel();
        delete oldDFmodel;
        delete m;
    }
    modelMap[name] = dfModel;
    if (prettyHeadersModel)
        view->setModel(prettyHeadersModel);
    else
        view->setModel(dfModel);
    view->verticalHeader()->hide();
    view->resizeColumnsToContents();
}

void DataframeViewer::showFindDialog()
{
    if (ui->currentItem().isEmpty())
        return;
    findDialog->show();
    findDialog->raise();
    findDialog->activateWindow();
}

void DataframeViewer::findForward(const QString &str, QFlags<QTextDocument::FindFlag> flags)
{
    Q_UNUSED(flags)
    QFlags<Qt::MatchFlag> flag;
//    if (flags |= QTextDocument::FindCaseSensitively)
//        flag = Qt::MatchCaseSensitive;
//    else
        flag = Qt::MatchExactly;
    QVariant qv(str);
    QString name = ui->currentItem();
    if (name.isEmpty())
        return;
    QTableView *view = viewsMap.values(name).at(0);
    DataFrameModel*dfModel = modelMap.value(name);

    // first try searching in the current column
    int row = view->currentIndex().row();
    int col = view->currentIndex().column();
    if (row<0)
        row=0;
    if (col<0)
        col=0;

    QModelIndexList hits = dfModel->match(dfModel->index(row, col),
                            Qt::DisplayRole,qv,1,flag);
    if (hits.size() == 0)
    {
        //now try a more systematic approach
        for (int i=0;i<dfModel->columnCount();i++)
        {
            hits = dfModel->match(dfModel->index(0, i),
                                Qt::DisplayRole,qv);
            if (hits.size() > 0)
                break;
        }
    }

    if (hits.size() > 0)
    {
        view->setCurrentIndex(hits.first());
//        findDialog->hide();
    }
    else
        QMessageBox::warning(this, "Find",QString("The text was not found"));
//        findDialog->hide();

}

void DataframeViewer::setPrettyHeadersForTrial(QString trial, QString df)
{
    PrettyHeadersModel *prettyHeadersModel = new PrettyHeadersModel(this);
    prettyHeadersModel->addHeaderReplaceRules(Qt::Horizontal, "event_pattern", "");
    prettyHeadersModel->addHeaderReplaceRules(Qt::Horizontal,"\\(", "");
    prettyHeadersModel->addHeaderReplaceRules(Qt::Horizontal,"\\)", "");
    prettyHeadersModel->addHeaderReplaceRules(Qt::Horizontal,trial, "");
    prettyHeadersModelMap.insert(df, prettyHeadersModel);
}

void DataframeViewer::dispatch()
{
    DataViewer::dispatch();
}

