#include "objectcataloguefilter.h"
#include "objectcatalogue.h"

#include <QDebug>
#include <QDomDocument>
Q_DECLARE_METATYPE(QDomDocument*)

ObjectCatalogueFilter::ObjectCatalogueFilter(ObjectCatalogue *objectCatalogue, QObject *parent)
    : objectCatalogue(objectCatalogue), QSortFilterProxyModel(parent)
{
    setSourceModel(objectCatalogue);
    connect(this, SIGNAL(rowsInserted(QModelIndex,int,int)),
            this, SLOT(sendObjectCreated(QModelIndex,int,int)));
    connect(this, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)),
            this, SLOT(sendObjectDestroyed(QModelIndex,int,int)));
}

bool ObjectCatalogueFilter::isAllValid()
{
    bool invalid(false);
    for (int row = 0; row < rowCount(); ++row)
    {
        invalid |= data(index(row,2)).toBool();
    }
    return !invalid;
}

void ObjectCatalogueFilter::setName(QString txt)
{
    setList(QStringList({txt}));
    setFilterKeyColumn(0);
}

void ObjectCatalogueFilter::setNameList(QStringList list)
{
    setList(list);
    setFilterKeyColumn(0);
}

void ObjectCatalogueFilter::setType(QString txt)
{
    setList(QStringList({txt}));
    setFilterKeyColumn(1);
}

void ObjectCatalogueFilter::setTypeList(QStringList list)
{
    setList(list);
    setFilterKeyColumn(1);
}

void ObjectCatalogueFilter::sendObjectCreated(QModelIndex parent, int first, int last)
{
    Q_UNUSED(parent)

    for (int row = first; row <= last; ++row)
    {
        QString name = data(index(row,0)).toString();
        QString type = data(index(row,1)).toString();
        qDebug() << "Name: " << name << " Type: " << type;
        QDomDocument* domDoc = objectCatalogue->description(name);
        emit objectCreated(name, type, domDoc);
    }
}

void ObjectCatalogueFilter::sendObjectDestroyed(QModelIndex parent, int first, int last)
{
    Q_UNUSED(parent)
    for (int row = first; row <= last; ++row)
    {
        QString name = data(index(row,0)).toString();
        qDebug () << "objectDestroyed" << name;
        emit objectDestroyed(name);
    }
}

void ObjectCatalogueFilter::setList(QStringList list)
{
    QRegExp rex = QRegExp(QString("^(%1)$").arg(list.replaceInStrings(QRegExp("([()])"), "\\\\1").join('|')));
    setFilterRegExp(rex);
}

