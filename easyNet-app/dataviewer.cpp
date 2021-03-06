#include "dataviewer.h"
#include "ui_dataviewer.h"
#include "sessionmanager.h"
#include "enumclasses.h"
#include "dataviewerdispatcher.h"
#include "objectcachefilter.h"
#include "objectupdater.h"
#include "xmlelement.h"


#include <QAction>
#include <QDomDocument>
#include <QVBoxLayout>
#include <QSharedPointer>

Q_DECLARE_METATYPE(QSharedPointer<QDomDocument> )

DataViewer::DataViewer(Ui_DataViewer *ui, QWidget *parent)
    : QWidget(parent), ui(ui), dispatcher(nullptr), m_lazy(false), m_name(""), m_itemPrettyName("")
{
    setUi();
    descriptionFilter = new ObjectCacheFilter(SessionManager::instance()->descriptionCache, this);
    connect(descriptionFilter, SIGNAL(objectDestroyed(QString)), this, SLOT(destroyItem(QString)));
    descriptionUpdater = new ObjectUpdater(this);
    descriptionUpdater->setCommand("description");
    descriptionUpdater->setProxyModel(descriptionFilter);
    connect(descriptionUpdater, SIGNAL(objectUpdated(QDomDocument*,QString)), this, SLOT(execAddItem(QDomDocument*,QString)));
    m_items.clear();
    hiddenItems.clear();
    setDefaultSaveDir(SessionManager::instance()->defaultLocation("outputDir"));
}

DataViewer::~DataViewer()
{
    //delete ui;
    //delete dispatcher;
}

void DataViewer::setDispatcher(DataViewerDispatcher *dataViewerDispatcher)
{
    Q_ASSERT(dataViewerDispatcher);
    dispatcher = dataViewerDispatcher;
}

void DataViewer::setUi()
{
    if (!ui)
    {
        eNerror << "invalid Ui_DataViewer object";
        return;
    }
//    qDebug()<<(ui->setup?"Y":"N");
//    if(!ui->setup)
    {
      ui->setupUi(this);
      QVBoxLayout *mlayout = new QVBoxLayout;
      mlayout->addWidget(ui);
      setLayout(mlayout);
     }

    enableActions(false);
}

void DataViewer::execAddItem(QDomDocument *domDoc, QString name)
{
    Q_UNUSED(domDoc)

    if (name.isEmpty())
    {
        eNerror << "name is empty";
        return;
    }

    if (contains(name))
    {
        if (dispatcher)
        {
            dispatcher->updateInfo(name);
            dispatcher->updateHistory(name);
//            if (SessionManager::instance()->visibility(name) == "1" && !ui->contains(name))
//            {
//                addView(name);
//                ui->setCurrentItem(name);
//            }
//            else if (SessionManager::instance()->visibility(name) == "0" && ui->contains(name))
//            {
//                removeView(name);
//            }
        }
        return;
    }
    else
    {
        addItem_impl(name);
        m_items.insert(name);
        if (isHidden(name))
        {
            addNameToFilter(name); // keep up to date but don't show in view
            return;
        }
        if (dispatcher)
        {
            QString visibility = SessionManager::instance()->visibility(name);
            dispatcher->addToHistory(name, visibility == "1");
            setFirstViewState(name);
        }
        else
        {
            addView(name);
        }
    }
}

void DataViewer::initiateDestroyItem(QString name)
{

    if (dispatcher && sender() == ui)
    {
        dispatcher->setInView(name, false);
    }
    else if (name.startsWith('(')) // don't destroy default dataframes
    {
        eNwarning << QString("attempt to delete lazyNut system object %1").arg(name);
        // this should change, trigger desable default observer
    }
    else
    {
        if (SessionManager::instance()->exists(name))
        {
            SessionManager::instance()->destroyObject(name);
        }
        else
        {
            SessionManager::instance()->removeFromExtraNamedItems(name);
            destroyItem(name);
        }
    }
}

void DataViewer::destroyItem(QString name)
{
    if (!contains(name))
    {
        eNwarning << QString("attempt to delete item %1 that does not belong to this viewer").arg(name);
        return;
    }
    if (dispatcher)
        dispatcher->removeFromHistory(name);
    destroyItem_impl(name);
    removeView(name);
    m_items.remove(name);
    hiddenItems.remove(name);
    emit itemRemoved(name);
}

bool DataViewer::contains(QString name)
{
    return m_items.contains(name);
}


void DataViewer::addView(QString name)
{
    ui->addView(name, makeView(name));
    if (!isLazy())
        addNameToFilter(name);

//    if (!dispatcher || (description && XMLelement(*description)["hints"]["show"]() == "1"))
//        ui->setCurrentItem(name);


}

void DataViewer::removeView(QString name)
{
    if (!isLazy())
        removeNameFromFilter(name);
    delete ui->takeView(name);
//    if (dispatcher)
//        SessionManager::instance()->setShowHint(name, "0");
    //    descriptionFilter->removeName(name);
}

QString DataViewer::currentItemName()
{
    return ui->currentItemName();
}

void DataViewer::resetViewStates()
{
    foreach (QString name, viewStateMap.keys())
    {
        if (viewStateMap.value(name) == ViewState_Fresh)
        {
            setViewState(name, ViewState_Stale);
        }
    }
}

void DataViewer::updateDependeesViewStates(QString depender)
{
    foreach (QString name, viewStateMap.keys())
    {
        if (viewStateMap.value(name) == ViewState_Stale &&
            SessionManager::instance()->dependencies(name).contains(depender))
        {
            setViewState(name, ViewState_Fresh);
        }
    }
}

void DataViewer::addItem(QString name, bool hidden)
{
    if (name.isEmpty())
    {
        QVariant v = SessionManager::instance()->getDataFromJob(sender(), "name");
        if (!v.canConvert<QString>())
        {
            eNerror << "cannot retrieve a valid string from name key in sender LazyNut job";
            return;
        }
        name = v.value<QString>();
        v = SessionManager::instance()->getDataFromJob(sender(), "hidden");
        if (v.canConvert<bool>())
            hidden = v.value<bool>();
    }
    if (name.isEmpty())
    {
        eNerror << "name is empty";
        return;
    }
    if (contains(name))
        return;

    if (hidden)
        hiddenItems.insert(name);

    if (SessionManager::instance()->exists(name))
    {
        descriptionFilter->addName(name);
    }
    else if (SessionManager::instance()->extraNamedItems().contains(name))
    {
        // e.g. plot snapshots do not correspond to any lazynut object
        execAddItem(nullptr, name);
    }
    else
    {
        // e.g. (hidden) reference parameter df does not appear in recently_*
        SessionManager::instance()->descriptionCache->create(name);
        descriptionFilter->addName(name);
    }
}


void DataViewer::preDispatch(QSharedPointer<QDomDocument> info)
{
    if (!info)
        eNerror << "not a valid trial run info pointer";
    else if (!dispatcher)
        eNerror << "received a trial run info object but no dispatcher set";
    else
        dispatcher->preDispatch(info);
}

void DataViewer::dispatch()
{
//    if (!info)
//    {
        QVariant infoVariant = SessionManager::instance()->getDataFromJob(sender(), "trialRunInfo");
        if (!infoVariant.canConvert<QSharedPointer<QDomDocument> >())
        {
            eNerror << "unable to get trial run information";
            return;
        }
        QSharedPointer<QDomDocument> info = infoVariant.value<QSharedPointer<QDomDocument> >();
//    }
    if (!dispatcher)
        eNerror << "received a trial run info object but no dispatcher set";
    else
        dispatcher->dispatch(info);
}

bool DataViewer::setCurrentItem(QString name)
{
    if (name.isEmpty() || name == "<select an item>"  || ! contains(name))
    {
        enableActions(false);
        return false;
    }
    else
    {
        enableActions(true);
        if (isLazy())
            setNameInFilter(name);
        ui->setToolBars(this);
    }
    emit currentItemChanged(name);
    return true;
}

void DataViewer::enableActions(bool enable)
{
    if (!ui)
        return;
    for(auto x: ui->actionSet[this])
        x->setEnabled(enable);

    if (dispatcher)
        dispatcher->enableActions(enable);

/*    ui->saveAct->setEnabled(enable);
    ui->copyAct->setEnabled(enable);
    ui->destroyAct->setEnabled(enable);
    ui->settingsAct->setEnabled(enable);
*/
}


void DataViewer::setTrialRunMode(int mode)
{
    if (dispatcher)
        dispatcher->setTrialRunMode(mode);
}


void DataViewer::destroySelectedItems()
{
    if (dispatcher && ui->historyAct->isChecked())
        dispatcher->destroySelectedItems();
    else
        initiateDestroyItem(ui->currentItemName());
}

