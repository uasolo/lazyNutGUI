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

#ifndef DIAGRAMSCENE_H
#define DIAGRAMSCENE_H

#include "diagramitem.h"
#include "diagramtextitem.h"
#include "arrow.h"

#include <QGraphicsScene>



QT_BEGIN_NAMESPACE
class QGraphicsSceneMouseEvent;
class QMenu;
class QPointF;
class QGraphicsLineItem;
class QFont;
class QGraphicsTextItem;
class QGraphicsItem;
class QColor;
class AsLazyNutObject;
class ObjectCatalogue;
class ObjectCatalogueFilter;
class DescriptionUpdater;
class QDomDocument;

QT_END_NAMESPACE

//! [0]
class DiagramScene : public QGraphicsScene
{
    Q_OBJECT

public:
    enum Mode { InsertItem, InsertLine, InsertText, MoveItem };

    explicit DiagramScene(QMenu *itemMenu, ObjectCatalogue *objectCatalogue,
                          QString boxType, QString arrowType, QObject *parent = 0);
    void setObjCatalogue(ObjectCatalogue *catalogue);
    QFont font() const { return myFont; }
    QColor textColor() const { return myTextColor; }
    QColor itemColor() const { return myItemColor; }
    QColor lineColor() const { return myLineColor; }
    void setLineColor(const QColor &color);
    void setTextColor(const QColor &color);
    void setItemColor(const QColor &color);
    void setFont(const QFont &font);
    void read(const QJsonObject &json);
    void write(QJsonObject &json) const;

public slots:
    void setMode(Mode mode);
    void setItemType(DiagramItem::DiagramType type);
    void setArrowTipType(Arrow::ArrowTipType type);
    void editorLostFocus(DiagramTextItem *item);
//    void syncToObjCatalogue();
    void setSelected(QString name);
    void savedLayoutToBeLoaded(QString _savedLayout);
    void saveLayout();
    void prepareToLoadLayout(QString fileName);


signals:
    void itemInserted(DiagramItem*);
    void textInserted(QGraphicsTextItem*);
    void itemSelected(QGraphicsItem*);
    void objectSelected(QString);
//    void showObj(LazyNutObj * obj, LazyNutObjCatalogue* objectCatalogue);
    void layoutSaveAttempted();


protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *mouseEvent);
    void mouseMoveEvent(QGraphicsSceneMouseEvent *mouseEvent);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *mouseEvent);
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *mouseEvent);

private slots:
    void positionObject(QString name, QString type, QDomDocument* domDoc);
    void removeObject(QString name);
    void renderObject(QDomDocument* domDoc);
    void loadLayout();

private:
    void render();
    bool isItemChange(int type);

    ObjectCatalogue *objectCatalogue;
    QHash<QString,QGraphicsItem*> itemHash;
    ObjectCatalogueFilter *objectFilter;
    DescriptionUpdater *descriptionUpdater;
    QList<QDomDocument*> renderList;

    QString boxType;
    QString arrowType;

    QString savedLayout;
    bool layoutLoaded = false;
    DiagramItem::DiagramType myItemType;
    Arrow::ArrowTipType myArrowTipType;
    QMenu *myItemMenu;
    Mode myMode;
    bool leftButtonDown;
    //QPointF startPoint;
    QPointF defaultPosition;
    QPointF currentPosition;
    QPointF itemOffset;
    QPointF arrowOffset;
    QGraphicsLineItem *line;
    QFont myFont;
    DiagramTextItem *textItem;
    QColor myTextColor;
    QColor myItemColor;
    QColor myLineColor;
};
//! [0]

#endif // DIAGRAMSCENE_H
