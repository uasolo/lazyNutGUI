#include "sessionmanager.h"
#include "commandsequencer.h"
#include "jobqueue.h"
#include "treeitem.h"
#include "treemodel.h"
#include "lazynutobj.h"
#include "querycontext.h"
#include "driver.h"
#include "lazynut.h"


#include <QtGlobal>
#include <QFinalState>
#include <QFileInfo>
#include <QBuffer>
#include <QDebug>
#include <QDomDocument>

class MacroQueue;
//typedef QHash<QString,LazyNutObj*> LazyNutObjCatalogue;



SessionManager::SessionManager(QObject *parent)
    : QObject(parent),
      lazyNutHeaderBuffer(""), lazyNutOutput(""), OOBrex("OOB secret: (\\w+)\\n")
{
    lazyNut = new LazyNut(this);
    macroQueue = new MacroQueue;

//    initParser(); // Bison, will be deleted
    startCommandSequencer();
}

//void SessionManager::initParser()
//{
//    context = new QueryContext;
//    driver = new lazyNutOutputParser::Driver(*context);
//}

void SessionManager::startLazyNut(QString lazyNutBat)
{

    connect(lazyNut,SIGNAL(outputReady(QString)),this,SLOT(getOOB(QString)));
    lazyNut->setWorkingDirectory(QFileInfo(lazyNutBat).absolutePath());
    lazyNut->start(lazyNutBat);
    // TODO: here we should have a wait until timeout, since lazyNut could be on a remote server
    if (lazyNut->state() == QProcess::NotRunning)
    {
        delete lazyNut;
        emit lazyNutNotRunning();
    }
}

void SessionManager::getOOB(const QString &lazyNutOutput)
{
    lazyNutHeaderBuffer.append(lazyNutOutput);
    if (lazyNutHeaderBuffer.contains(OOBrex))
    {
        OOBsecret = OOBrex.cap(1);
        qDebug() << OOBsecret;
        lazyNutHeaderBuffer.clear();
        disconnect(lazyNut,SIGNAL(outputReady(QString)),this,SLOT(getOOB(QString)));
    }
}

void SessionManager::startCommandSequencer()
{
    commandSequencer = new CommandSequencer(lazyNut, this);
    connect(commandSequencer,SIGNAL(userLazyNutOutputReady(QString)),
            this,SIGNAL(userLazyNutOutputReady(QString)));
//    connect(commandSequencer,SIGNAL(commandsExecuted()),this,SIGNAL(commandsExecuted()));
    connect(commandSequencer,SIGNAL(recentlyModifiedReady(QStringList)),
            this,SLOT(updateRecentlyModified(QStringList)));
    connect(commandSequencer,SIGNAL(descriptionReady(QDomDocument*)),
            this,SIGNAL(descriptionReady(QDomDocument*)));

}


//bool SessionManager::parseLazyNutOutput()
//{
//    return driver->parse_string(lazyNutOutput.toStdString(), "lazyNutOutput");
//}

//void SessionManager::updateObjects()
//{
//    bool objHashModified = false;
//    foreach (TreeItem* queryItem, context->root->children())
//    {
//        QString queryType = queryItem->data(0).toString();
//        if (queryType == "subtypes")
//        {
//            QString objectType = queryItem->data(1).toString();
//            QModelIndex objectIndex = objTaxonomyModel->index(0,0);
//            int row = 0;
//            while (objTaxonomyModel->data(objTaxonomyModel->index(row,0,objectIndex),Qt::DisplayRole).toString() != objectType &&
//                   row < objTaxonomyModel->rowCount(objectIndex))
//                ++row;
//            QModelIndex typeIndex = objTaxonomyModel->index(row,0,objectIndex);
//            if (typeIndex.isValid())
//            {
//                foreach (TreeItem* subtypeItem, queryItem->children())
//                {
//                    objTaxonomyModel->appendValue(subtypeItem->data(1).toString(),typeIndex);
//                }
//            }
//        }
//        else if (queryType == "recently_modified")
//        {
//            foreach (TreeItem* objectItem, queryItem->children())
//            {
//                recentlyModified.append(objectItem->data(1).toString());
//            }
//        }
//        else if (queryType == "description")
//        {
//            emit beginObjHashModified();
//            foreach (TreeItem* objectItem, queryItem->children())
//            {
//                QString objectName = objectItem->data(1).toString();
//                objCatalogue->insert(objectName,new LazyNutObj());
//                foreach (TreeItem* propertyItem, objectItem->children())
//                {
//                    QString propertyKey = propertyItem->data(0).toString();
//                    QString propertyValue = propertyItem->data(1).toString();
//                    if (propertyValue.startsWith('[') && propertyValue.endsWith(']'))
//                        // todo: generate query list
//                        (*objCatalogue)[objectName]->appendProperty(propertyKey,propertyValue);
//                    else if (propertyValue.contains(','))
//                        (*objCatalogue)[objectName]->appendProperty(propertyKey,propertyValue.split(", "));
//                    else
//                        (*objCatalogue)[objectName]->appendProperty(propertyKey,propertyValue);
//                }
//            }
//            objHashModified = true;
//        }
//    }
//    if (objHashModified)
//        emit endObjHashModified();
//    context->clearQueries();
//}





void SessionManager::updateRecentlyModified(QStringList _recentlyModified)
{
    recentlyModified.append(_recentlyModified);
}


void SessionManager::killLazyNut()
{
    lazyNut->kill();
}


void SessionManager::runCommands()
{
    commandSequencer->runCommands(commandList, JobOrigin::User);
}

QStateMachine *SessionManager::buildMacro()
{
    QStateMachine * macro = new QStateMachine(this);
    connect(macro,SIGNAL(started()),this,SLOT(macroStarted()));
    connect(macro,SIGNAL(finished()),this,SLOT(macroEnded()));
    connect(macro,SIGNAL(finished()),macro,SLOT(deleteLater()));
    return macro;
}


void SessionManager::runModel(QStringList cmdList)
{
    commandList = cmdList;
    QStateMachine * macro = buildMacro();
    // states
    QState * runCommandsState = new QState(macro);
    connect(runCommandsState,SIGNAL(entered()),this,SLOT(runCommands()));
    QState * getSubtypesState = new QState(macro);
    connect(getSubtypesState,SIGNAL(entered()),this,SLOT(getSubtypes()));
    QState * getRecentlyModifiedState = new QState(macro);
    connect(getRecentlyModifiedState,SIGNAL(entered()),this,SLOT(getRecentlyModified()));
    QState * getDescriptionsState = new QState(macro);
    connect(getDescriptionsState,SIGNAL(entered()),this,SLOT(getDescriptions()));
    connect(getDescriptionsState,SIGNAL(exited()),this,SIGNAL(updateDiagramScene()));

    QFinalState *finalState = new QFinalState(macro);
    macro->setInitialState(runCommandsState);

    // transitions
    runCommandsState->addTransition(commandSequencer,SIGNAL(commandsExecuted()),getSubtypesState);
    getSubtypesState->addTransition(commandSequencer,SIGNAL(commandsExecuted()),getRecentlyModifiedState);
    getRecentlyModifiedState->addTransition(commandSequencer,SIGNAL(commandsExecuted()),getDescriptionsState);
    getDescriptionsState->addTransition(commandSequencer,SIGNAL(commandsExecuted()),finalState);
    getDescriptionsState->addTransition(this,SIGNAL(skipDescriptions()),finalState);

    macroQueue->tryRun(macro);
}

void SessionManager::runSelection(QStringList cmdList)
{
    commandList = cmdList;
    QStateMachine * macro = buildMacro();
    // states
    QState * runCommandsState = new QState(macro);
    connect(runCommandsState,SIGNAL(entered()),this,SLOT(runCommands()));
    QState * getRecentlyModifiedState = new QState(macro);
    connect(getRecentlyModifiedState,SIGNAL(entered()),this,SLOT(getRecentlyModified()));
    QState * getDescriptionsState = new QState(macro);
    connect(getDescriptionsState,SIGNAL(entered()),this,SLOT(getDescriptions()));
    connect(getDescriptionsState,SIGNAL(exited()),this,SIGNAL(updateDiagramScene()));
    QFinalState *finalState = new QFinalState(macro);
    macro->setInitialState(runCommandsState);

    // transitions
    runCommandsState->addTransition(commandSequencer,SIGNAL(commandsExecuted()),getRecentlyModifiedState);
    getRecentlyModifiedState->addTransition(commandSequencer,SIGNAL(commandsExecuted()),getDescriptionsState);
    getDescriptionsState->addTransition(commandSequencer,SIGNAL(commandsExecuted()),finalState);
    getDescriptionsState->addTransition(this,SIGNAL(skipDescriptions()),finalState);

    macroQueue->tryRun(macro);
}



bool SessionManager::getStatus()
{
    return commandSequencer->getStatus();
}

void SessionManager::pause()
{
    macroQueue->pause();
//    commandSequencer->pause();
    emit isPaused(macroQueue->isPaused());
}

void SessionManager::stop()
{
    macroQueue->stop();
//    commandSequencer->stop();
}


void SessionManager::getSubtypes()
{
    commandList.clear();
    foreach (QString type, lazyNutObjTypes)
        commandList.append(QString("xml subtypes %1").arg(type));
    commandSequencer->runCommands(commandList, JobOrigin::GUI);
}

void SessionManager::getRecentlyModified()
{
    commandList.clear();
    commandList << "xml recently_modified"
                << "clear_recently_modified";
    commandSequencer->runCommands(commandList, JobOrigin::GUI);
}

void SessionManager::clearRecentlyModified()
{
    commandList.clear();
    commandList << "clear_recently_modified";
    commandSequencer->runCommands(commandList, JobOrigin::GUI);
}

void SessionManager::getDescriptions()
{
    if (recentlyModified.isEmpty())
        emit skipDescriptions();
    else
    {
        commandList.clear();
        foreach (QString obj, recentlyModified)
            commandList.append(QString("xml %1 description").arg(obj));
        commandSequencer->runCommands(commandList, JobOrigin::GUI);
        recentlyModified.clear();
    }
}

void SessionManager::macroStarted()
{
    qDebug() << "Macro started.";
}

void SessionManager::macroEnded()
{
    qDebug() << "Macro ended.";
    macroQueue->freeToRun();
}



void MacroQueue::run(QStateMachine *macro)
{
    qDebug() << "Jobs in MacroQueue:" << queue.size();
    macro->start();
}

void MacroQueue::reset()
{
    qDebug() << "RESET CALLED, Jobs in MacroQueue:" << queue.size();
    while (!queue.isEmpty())
    {
        delete queue.dequeue();
    }
    if (currentJob)
        currentJob->stop();
    //delete currentJob;
}

QString MacroQueue::name()
{
    return "MacroQueue";
}
