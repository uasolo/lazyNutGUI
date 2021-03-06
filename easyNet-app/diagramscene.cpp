/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
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
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
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
** $QT_END_LICENSE$
**
****************************************************************************/

#include "diagramscene.h"
#include "arrow.h"
#include "lazynutobject.h"
#include "objectcachefilter.h"
#include "objectupdater.h"
#include "sessionmanager.h"
#include "objectcache.h"
#include "lazynutjob.h"
#include "box.h"
#include "arrow.h"


#include <algorithm>


#include <QTextCursor>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSceneContextMenuEvent>
#include <QMenu>
#include <QAction>
#include <QJsonArray>
#include <QJsonDocument>
#include <QFileInfo>
#include <QMetaObject>
#include <QDomDocument>
#include <QDebug>
#include <QUndoStack>
#include <QPainter>

Q_DECLARE_METATYPE(QDomDocument*)

//! [0]
DiagramScene::DiagramScene(QString modeln,QString box_type, QString arrow_type, QObject *parent)
    : QGraphicsScene(parent), m_boxType(box_type), m_arrowType(arrow_type), awake(true), m_layoutFile(""),
      m_newModelLoaded(false),m_modelname(modeln)
{
    selectedObject = "";
//    myItemMenu = itemMenu;
//    myMode = MoveItem;
//    myItemType = QGraphicsItem::Layer;
//    line = 0;
//    textItem = 0;
//    myItemColor = Qt::white;
//    myTextColor = Qt::black;
//    myLineColor = Qt::black;

    zebimage = new QImage();
    zebimage->load(":/images/faint.zeb.3.png");
//    zebimage->load(":/images/zebra.png");



    setNewModelLoaded(false);

    setProperty("structuralEditingDisabled", true);
    setProperty("idealEdgeLengthModifier", 1.0);
    setProperty("preventOverlaps", true);

    // box parameters
    boxLongNameToDisplayIntact = "longnameXX";
    boxWidthMarginProportionToLongestLabel = 0.1;
    boxWidthOverHeight = 1.618;
    // compute box width and use it to set shapeNonOverlapPadding
    QFontMetrics fm(canvasFont());
    qreal boxWidth = (1.0 + 2.0 * boxWidthMarginProportionToLongestLabel) * fm.width(boxLongNameToDisplayIntact);
    int shapeNonOverlapPadding = boxWidth * 0.4; // just an estimate
    qreal idealConnectorLength = boxWidth * 2.0; // just an estimate
    jitter = boxWidth * 0.3; // just an estimate

    setProperty("idealConnectorLength", idealConnectorLength);
    setProperty("shapeNonOverlapPadding", shapeNonOverlapPadding);

    defaultPosition = QPointF(0,0); //  150);
    currentPosition = defaultPosition;
    itemOffset = QPointF(0,50) ; // 150);
    arrowOffset = QPointF(50,0);

    modelFilter = new ObjectCacheFilter(SessionManager::instance()->descriptionCache, this);
//    connect(SessionManager::instance(), SIGNAL(currentModelChanged(QString)),
//            modelFilter, SLOT(setName(QString)));
//    connect(modelFilter, SIGNAL(objectCreated(QString,QString,QString,QDomDocument*)),
//            this, SLOT(goToSleep()));
    modelFilter->setName(m_modelname);
    modelDescriptionUpdater = new ObjectUpdater(this);
    modelDescriptionUpdater->setProxyModel(modelFilter);
    connect(modelDescriptionUpdater, SIGNAL(objectUpdated(QDomDocument*,QString)),
            this, SLOT(setMembers(QDomDocument*)));
    connect(modelDescriptionUpdater, SIGNAL(objectUpdated(QDomDocument*,QString)),
            this, SLOT(setLayoutFile(QDomDocument*)));
    connect(modelFilter, &ObjectCacheFilter::objectDestroyed, [=]()
    {
        m_layoutFile = "";
//        setNewModelLoaded(false);
    });

    connect(SessionManager::instance(), &SessionManager::modelStageCompleted, [=]()
    {
        setNewModelLoaded(true);
        render();
        qDebug()<<"RR";
    });

    boxFilter = new ObjectCacheFilter(SessionManager::instance()->descriptionCache, this);

    arrowFilter = new ObjectCacheFilter(SessionManager::instance()->descriptionCache, this);

    boxDescriptionUpdater = new ObjectUpdater(this);
    boxDescriptionUpdater->setProxyModel(boxFilter);

    arrowDescriptionUpdater = new ObjectUpdater(this);
    arrowDescriptionUpdater->setProxyModel(arrowFilter);

    connect(boxDescriptionUpdater, SIGNAL(objectUpdated(QDomDocument*, QString)),
            this, SLOT(renderObject(QDomDocument*)));

    connect(arrowDescriptionUpdater, SIGNAL(objectUpdated(QDomDocument*, QString)),
            this, SLOT(renderObject(QDomDocument*)));
    //connect(m_animation_group, SIGNAL(finished()), this, SIGNAL(animationFinished()));

//    syncToObjCatalogue();
    // default state is wake up
    connect(boxFilter, SIGNAL(objectDestroyed(QString)),
            this, SLOT(removeObject(QString)));
    connect(arrowFilter, SIGNAL(objectDestroyed(QString)),
            this, SLOT(removeObject(QString)));


//    wakeUp();
    QPixmap zebpix(150,200);

    zebpix.load(":/images/zebra.png");
qDebug()<<"ZEB";
}

void DiagramScene::drawBackground(QPainter *painter, const QRectF &rect)
{
    painter->save();
    painter->drawImage(rect, *zebimage);
    painter->restore();
}

// This implementation of the connected component algorithm is adapted from:
// https://breakingcode.wordpress.com/2013/04/08/finding-connected-components-in-a-graph/
QList<QSet<DiagramItem *> > DiagramScene::connectedComponents()
{
    QList<QSet<DiagramItem *> > cc;
    QSet<DiagramItem *> shapeSet = shapes().toSet();
    while (!shapeSet.isEmpty())
    {
        DiagramItem * shape = shapeSet.toList().first();
        shapeSet.remove(shape);
        QSet<DiagramItem *> group({shape});
        QList<DiagramItem *> queue({shape});
        while (!queue.isEmpty())
        {
            QSet<DiagramItem *> neighbourSet = queue.takeFirst()->neighbours();
            neighbourSet.subtract(group);
            shapeSet.subtract(neighbourSet);
            group.unite(neighbourSet);
            queue.append(neighbourSet.toList());
        }
        cc.append(group);
    }
    return cc;
}

QList<DiagramItem *> DiagramScene::shapes()
{
    QList<DiagramItem *> result;
    QListIterator<QGraphicsItem*> it(items());
    while(it.hasNext())
    {
        DiagramItem *shape = dynamic_cast<DiagramItem*>(it.next());
        if (shape)
            result.append(shape);
    }
    return result;
}

QList<Box *> DiagramScene::boxes()
{
    QList<Box *> result;
    QListIterator<QGraphicsItem*> it(items());
    while(it.hasNext())
    {
        Box *box = dynamic_cast<Box*>(it.next());
        if (box)
            result.append(box);
    }
    return result;
}

void DiagramScene::updateConnectorsForLayout()
{
    for(auto x:items())
    {
        if(auto arrow=dynamic_cast<Arrow*>(x))
            arrow->updatePosition();
    }
}

void DiagramScene::read(const QJsonObject &json)
{
    qDebug()<<"jsonread "<<m_layoutFile<<" vs "<<itemHash.size();
    qreal boxWidth = json["boxWidth"].toDouble();
    QJsonArray itemArray = json["QGraphicsItems"].toArray();

    for (int itemIndex = 0; itemIndex < itemArray.size(); ++itemIndex)
    {
        QJsonObject itemObject = itemArray[itemIndex].toObject();
        QString name = itemObject["name"].toString();
        auto i=itemHash.keyBegin();
        QString k;
        for(;i!=itemHash.keyEnd();++i)
        {
            auto x=(*i);
            x.replace(QRegExp(".*::"),"");
            if(x==name) break;
        }
        if (i!=itemHash.keyEnd())
        {
            Box * box = dynamic_cast<Box*>(itemHash.value(*i));
            if (box)
                box->read(itemObject, boxWidth);
        }
    }
    QJsonArray itemArray2 = json["diagramItems"].toArray();
    for (int itemIndex = 0; itemIndex < itemArray2.size(); ++itemIndex)
    {
        QJsonObject itemObject = itemArray2[itemIndex].toObject();
        QString name = itemObject["name"].toString();
        auto i=itemHash.keyBegin();
        QString k;
        for(;i!=itemHash.keyEnd();++i)
        {
            auto x=k=(*i);
            x.replace(QRegExp(".*::"),"");
            if(x==name) break;
        }
        if (i!=itemHash.keyEnd())
        {
            Box * box = dynamic_cast<Box*>(itemHash.value(k));
            if (box)
                box->read(itemObject, boxWidth);
        }
    }
    qDebug()<<"b";
    emit animationFinished();

}

void DiagramScene::write(QJsonObject &json)
{
    QJsonArray itemArray;
    qreal boxWidth=0;
    foreach (Box * box, boxes())
    {
            QJsonObject itemObject;
            box->write(itemObject);
            itemArray.append(itemObject);
            if (boxWidth == 0)
                boxWidth = box->width();
    }
    json["QGraphicsItems"] = itemArray;
    json["boxWidth"] = boxWidth;
}


bool DiagramScene::validForAlignment(QList<Box *> items)
{
    return true;
}

void DiagramScene::alignSelection(DiagramScene::Alignment al)
{
  qreal average=0;
  int n=0;
  for(auto& item:selectedItems())
  {
      Box *box = dynamic_cast<Box *>(item);
      if (box)
      {
          qreal coord=(al==DiagramScene::Horizontal)?box->centrePos().x():box->centrePos().y();
          average+=(coord-average)/(++n);
      }
  }
  for(auto& item:selectedItems())
  {
      Box *box = dynamic_cast<Box *>(item);
      if (box)
      {
          qreal coord=(al==DiagramScene::Horizontal)?box->centrePos().x():box->centrePos().y();
          qreal xchange=(al==DiagramScene::Horizontal)?average-coord:0;
          qreal ychange=(al==DiagramScene::Horizontal)?0:average-coord;
          box->moveBy(xchange,ychange);
      }
  }
  updateConnectorsForLayout();
}

void DiagramScene::deleteSelection()
{

}

//! [4]

//    if (!layoutLoaded)
//    {
//        QFile savedLayoutFile(savedLayout);
//        if (savedLayoutFile.open(QIODevice::ReadOnly))
//        {
//            QByteArray savedLayoutData = savedLayoutFile.readAll();
//            QJsonDocument savedLayoutDoc(QJsonDocument::fromJson(savedLayoutData));
//            read(savedLayoutDoc.object());
//        }
//        //layoutLoaded = true;
//    }
//}

#if 0
void DiagramScene::prepareToLoadLayout(QString fileName)
{
    savedLayout = fileName;
//    connect(descriptionUpdater, SIGNAL(objectUpdated(QDomDocument*)),
//            this, SLOT(loadLayout()));
}
#endif


void DiagramScene::setSelected(QString name)
{
#if 0
    if (itemHash.contains(name))
        setSelection(QList<QGraphicsItem*>{itemHash.value(name)});
#endif
}

void DiagramScene::initShapePlacement()
{
    // Connected component (CC) centres will be placed on a square grid of S points,
    // where S is the square number closest to the number of CCs.
    // The distance between CCs is estimated by supposing that the nodes in a CC will be
    // displayed in a square grid of S points, where S is defined as above.
    // The distance between nodes is taken as the ideal distance parameter.
    // The distance between CCs is computed based on the CC that contains the largest
    // number of nodes.


    LazyNutJob *job = new LazyNutJob;
    job->cmdList << QString("xml %1 analyze").arg(SessionManager::instance()->currentModel());
    job->setAnswerReceiver(this, SLOT(setAnalyzedLocations(QDomDocument*)), AnswerFormatterType::XML);
    SessionManager::instance()->submitJobs(job);

    return;
}

void DiagramScene::setAnalyzedLocations(QDomDocument *domDoc)
{
qDebug()<<"analyzing";
    foreach(Box*box, boxes())
    {
        QString n=box->name();

        XMLelement xm=XMLelement(*domDoc)["layers"].firstChild();
        do
        {
//            qDebug()<<xm.label()<<" "<<xm.value();
            if(xm.value()==n)
            {
                double x =2*box->width()*xm["breadth"].value().toDouble();
                double y= -2*box->height()*xm["depth"].value().toDouble();
                box->setCentrePos(QPointF(x,y));
            }
            xm=xm.nextSibling();
        }while(!xm.isNull());
    }
#if 0
    layout()->initialise();
#endif
    updateConnectorsForLayout();
qDebug()<<"A";
    emit animationFinished();
}

void DiagramScene::wakeUp()
{
//    if (!awake)
//    {
        qDebug() << Q_FUNC_INFO << "OBSOLETE";
         awake = true;
//        connect(boxFilter, SIGNAL(objectCreated(QString, QString, QString, QDomDocument*)),
//                this, SLOT(positionObject(QString, QString, QString, QDomDocument*)));
//        connect(boxFilter, SIGNAL(objectDestroyed(QString)),
//                this, SLOT(removeObject(QString)));
//        connect(arrowFilter, SIGNAL(objectDestroyed(QString)),
//                this, SLOT(removeObject(QString)));
        boxDescriptionUpdater->wakeUpUpdate();
        arrowDescriptionUpdater->wakeUpUpdate();
        syncToObjCatalogue();

//    }
}

void DiagramScene::goToSleep()
{
    if (awake)
    {
        qDebug() << Q_FUNC_INFO  << "OBSOLETE";
//        disconnect(boxFilter, SIGNAL(objectCreated(QString, QString, QString, QDomDocument*)),
//                   this, SLOT(positionObject(QString, QString, QString, QDomDocument*)));
//        disconnect(boxFilter, SIGNAL(objectDestroyed(QString)),
//                   this, SLOT(removeObject(QString)));
//        disconnect(arrowFilter, SIGNAL(objectDestroyed(QString)),
//                   this, SLOT(removeObject(QString)));
        boxDescriptionUpdater->goToSleep();
        arrowDescriptionUpdater->goToSleep();
        awake = false;
    }
}

void DiagramScene::disableAllObservers()
{
    foreach(Box *box, boxes())
        box->disableAllObservers();
}


#if 0
void DiagramScene::savedLayoutToBeLoaded(QString _savedLayout)
{
    savedLayout = _savedLayout;
    layoutLoaded = !(QFileInfo(savedLayout).exists());
}

#endif


//void DiagramScene::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *mouseEvent)
//{
//    if (mouseEvent->button() != Qt::LeftButton)
//        return;
//    foreach (QGraphicsItem *item, QGraphicsScene::items(mouseEvent->scenePos()))
//    {
//        if(item)
//        {
//            auto selectedObject = itemHash.key(item);
//            if(item->parentItem()) selectedObject=itemHash.key(item->parentItem());
//            emit objectSelected(selectedObject);
//            break;
//        }
//    }
//}

void DiagramScene::considerPositionObject(QString name, QString type, QString subtype, QDomDocument *domDoc)
{
    if(name.startsWith(m_modelname+"::"))
    {
        boxFilter->addName(name);
        positionObject(name,type,subtype,domDoc);
    }
}

void DiagramScene::positionObject(QString name, QString type, QString subtype, QDomDocument *domDoc)
{
    qDebug()<<"position "<<name<<type<<subtype<<domDoc;
    // layers are placed on the scene before arrows
   // Q_UNUSED(domDoc)
    Q_UNUSED(subtype)
    if (!awake)
        return;
    if (type == m_boxType)
    {
        Box *box = new Box();
        connect(box, SIGNAL(createDataViewRequested(QString,QString,QString,QMap<QString, QString>,bool)),
                this, SIGNAL(createDataViewRequested(QString,QString,QString,QMap<QString,QString>,bool)));

        addItem(box);
        box->setName(name); // set name before type, otherwise defaultDataframesFilter won't get properly set for layers
        box->setLazyNutType(m_boxType);
        connect(box, &Box::propertiesRequested, [=]()
        {
            qDebug()<<"pr"<<box->name();
            emit propertiesRequested(box->name());
        });
        connect(box, SIGNAL(focusOnPlotRequested(QString)), this, SIGNAL(focusOnPlotRequested(QString)));
        connect(box, SIGNAL(plotDestroyed(QString)), this, SIGNAL(plotDestroyed(QString)));
//        if (m_boxType == "layer")
//            connect(boxDescriptionUpdater, SIGNAL(objectUpdated(QDomDocument*, QString)),
//                    box, SLOT(cacheFromDescription(QDomDocument*,QString)));



        // temporarily define dimensions here

//        box->setProperty("position", defaultPosition);
        box->setCentrePos(defaultPosition);
        box->setProperty("longNameToDisplayIntact", boxLongNameToDisplayIntact);
        box->setProperty("widthMarginProportionToLongestLabel", boxWidthMarginProportionToLongestLabel);
        box->setProperty("widthOverHeight", boxWidthOverHeight);
//        box->setLabelPointSize(18);
//        box->autoSize();
//        box->setLabel(name);
        box->setToolTip(name);
//        if (m_boxType == "representation")
//            box->setFillColour(QColor("azure"));

        itemHash.insert(name,box);
    }
}

void DiagramScene::removeObject(QString name)
{
    if (!awake)
        return;
   QGraphicsItem* item = itemHash.value(name);
    if (!item)
        return;
#if 0
    setSelection(QList<QGraphicsItem*>{item});
    setProperty("structuralEditingDisabled", false);
    deleteSelection();
    setProperty("structuralEditingDisabled", true);
 #endif
    delete item;
    itemHash.remove(name);


}

void DiagramScene::renderObject(QDomDocument *domDoc)
{
    QString name=AsLazyNutObject(*domDoc).name();
    if(itemHash.find(name)==itemHash.end())
    {
        QString type=AsLazyNutObject(*domDoc).type();
        QString subtype=AsLazyNutObject(*domDoc).subtype();
        positionObject(name,type,subtype,domDoc);
    }

    // wait until all descriptions of recently_* objects have arrived
    renderList.append(domDoc);
    // HACK to allow visualisation of conversions and representations even when missing 'ghost' objects are present
//    if ((boxFilter->isAllValid() && arrowFilter->isAllValid()) || boxType() == "representation")
    render();
}

void DiagramScene::setMembers(QDomDocument *domDoc)
{
    boxFilter->setNameList(AsLazyNutObject(*domDoc)["members"][m_boxType+"s"].listValues());
    arrowFilter->setNameList(AsLazyNutObject(*domDoc)["members"][m_arrowType+"s"].listValues());
}

void DiagramScene::setLayoutFile(QDomDocument *domDoc)
{
    // temporarily, only layer plots are considered
    if (domDoc && m_boxType == "layer")
    {
        m_layoutFile = XMLelement(*domDoc)["hints"]["json"]();
        qDebug()<<"layout "<<m_layoutFile;
        if (m_layoutFile.isEmpty())
            m_layoutFile = QString("%1/Models/%2/%2.layer.json")
                    .arg(SessionManager::instance()->easyNetDataHome())
                    .arg(SessionManager::instance()->currentModel());
        if (QFileInfo(m_layoutFile).isRelative())
            m_layoutFile.prepend(SessionManager::instance()->easyNetDataHome()+"/");
//        render();
    }
}


void DiagramScene::render()
{
    if(itemHash.empty())return;
    if (!(boxFilter->isAllValid() &&
          arrowFilter->isAllValid()
          &&
          ((!layoutFile().isEmpty() && SessionManager::instance()->isModelStageCompleted()) != SessionManager::instance()->currentModel().isEmpty())
          )
        )
        return;
    foreach(QDomDocument* domDoc, renderList)
    {
        if (!domDoc)
        {
            qDebug()<<"render 0";
            continue;
        }
        if (AsLazyNutObject(*domDoc).type() == m_boxType)
        {
            QString name = AsLazyNutObject(*domDoc).name();
            qDebug()<<"render box "<<name;
            QMap <QString, QString> ports;
            foreach (QString label, XMLelement(*domDoc)["Ports"].listLabels())
                ports[label] = XMLelement(*domDoc)["Ports"][label]();
            QString pname = XMLelement(*domDoc)["pretty name"]();
            QString boxColor = XMLelement(*domDoc)["hints"]["color"]();
            Box* box = static_cast<Box*>(itemHash.value(name));

            box->setDashedStroke(AsLazyNutObject(*domDoc).lesioned());
            box->setPorts(ports);
            box->setLabel(pname);
            if (!boxColor.isEmpty())
                box->setColour(boxColor);
        }
        else if (AsLazyNutObject(*domDoc).type() == m_arrowType)
        {
            QString name = AsLazyNutObject(*domDoc).name();
            Box *startItem = qgraphicsitem_cast<Box *>
                    (itemHash.value(AsLazyNutObject(*domDoc)["Source"]()));
            Box *endItem = qgraphicsitem_cast<Box *>
                    (itemHash.value(AsLazyNutObject(*domDoc)["Target"]()));
            Arrow *arrow;
            bool isNew=false;
            if (itemHash.contains(name))
                arrow = qgraphicsitem_cast<Arrow*>(itemHash.value(name));
            else
            {
                isNew=true;
                arrow = new Arrow();
                arrow->setName(name);
                arrow->setLazyNutType(m_arrowType);
                arrow->setToolTip(name);
                arrow->setDashedStroke(SessionManager::instance()->descriptionCache->subtype(name) == "lesioned_connection");
                connect(arrow, &Arrow::propertiesRequested, [=]()
                {
                    emit propertiesRequested(arrow->name());
                });
            }

            if (!startItem && !endItem)
            {
                arrow->setNewEndpoint(Arrow::SRCPT, defaultPosition, nullptr);
                arrow->setNewEndpoint(Arrow::DSTPT, defaultPosition - QPointF(0, 50), nullptr);
//                currentPosition += arrowOffset;
//                arrow->setArrowStart(currentPosition);
            }
            else if (!startItem)
            {
                arrow->setNewEndpoint(Arrow::DSTPT, endItem->centrePos(), endItem,Arrow::CENTRE_CONNECTION_PIN);
                arrow->setNewEndpoint(Arrow::SRCPT, endItem->centrePos() + QPointF(0,endItem->height()/2 + 50), nullptr);
            }
            else if (!endItem)
            {
                arrow->setNewEndpoint(Arrow::SRCPT, startItem->centrePos(), startItem, Arrow::CENTRE_CONNECTION_PIN);
                arrow->setNewEndpoint(Arrow::DSTPT, startItem->centrePos() - QPointF(0,startItem->height()/2 + 50), nullptr);
            }
            else
            {
                if(isNew)
                {
                    arrow->initWithConnection(startItem, endItem);
                }
                else
                {
                    arrow->setNewEndpoint(Arrow::SRCPT, startItem->centrePos(), startItem, Arrow::CENTRE_CONNECTION_PIN);
                    arrow->setNewEndpoint(Arrow::DSTPT, endItem->centrePos(), endItem, Arrow::CENTRE_CONNECTION_PIN);
                }
            }

            if (!itemHash.contains(name))
            {
                addItem(arrow);
                auto ptr=dynamic_cast<QGraphicsItem*>(arrow);
                itemHash.insert(name,ptr);
            }
            arrow->setDashedStroke(AsLazyNutObject(*domDoc).lesioned());
        }
    }
    renderList.clear();
#if 0
    layout()->initialise();
#endif
    updateConnectorsForLayout();

    if (newModelLoaded())
    {
        setNewModelLoaded(false);
        qDebug()<<"newmodelpath";
        emit initArrangement();
    }
}


//! [13]

//! [14]
#if 0
bool DiagramScene::isItemChange(int type)
{
    foreach (QGraphicsItem *item, selectedItems()) {
        if (item->type() == type)
            return true;
    }
    return false;
}
#endif
//! [14]

void DiagramScene::syncToObjCatalogue()
{
    qDebug() << Q_FUNC_INFO << "OBSOLETE";
    QString name;
    QString subtype;
    // display new layers, hold new connections in a list
    for (int row=0;row<boxFilter->rowCount();row++)
    {
        name = boxFilter->data(boxFilter->index(row,ObjectCache::NameCol)).toString();
        subtype = boxFilter->data(boxFilter->index(row,ObjectCache::SubtypeCol)).toString();
        if (!itemHash.contains(name))
        {
            positionObject(name, m_boxType, subtype, nullptr);
            QVariant v = boxFilter->data(boxFilter->index(row, ObjectCache::DomDocCol));
            if (v.canConvert<QDomDocument *>())
                renderList.append(v.value<QDomDocument *>());
            boxDescriptionUpdater->requestObject(name);
        }
//        else if ((boxFilter->data(boxFilter->index(row,1)).toString() == m_arrowType))
//            &&   (!itemHash.contains(name)))
//            newConnections << name;
    }

    //    display new connections
    for (int row=0;row<arrowFilter->rowCount();row++)
    {
        name = arrowFilter->data(arrowFilter->index(row,0)).toString();
        if (!itemHash.contains(name))
        {
            connections.append(name); // OBSOLETE
            QVariant v = arrowFilter->data(arrowFilter->index(row, ObjectCache::DomDocCol));
            if (v.canConvert<QDomDocument *>())
                renderList.append(v.value<QDomDocument *>());
            arrowDescriptionUpdater->requestObject(name);
        }
    }

   # if 0
    foreach(QString name, newConnections)
    {
        connections.append(name);
        QModelIndexList matchedIndexList = objectFilter->match(objectFilter->index(0,0),Qt::DisplayRole,name);
        QVariant v = objectFilter->data(objectFilter->index(matchedIndexList.at(0).row(),3));
        if (v.canConvert<QDomDocument *>())
            renderList.append(v.value<QDomDocument *>());
        arrowDescriptionUpdater->requestObject(name);
    }
#endif
    // remove destroyed objects
    foreach (QString name, itemHash.keys())
    {
        QModelIndexList matchedIndexList = boxFilter->match(boxFilter->index(0,0),Qt::DisplayRole,name);
        matchedIndexList.append(arrowFilter->match(arrowFilter->index(0,0),Qt::DisplayRole,name));
        if (matchedIndexList.isEmpty())
            removeObject(name);
    }

}
