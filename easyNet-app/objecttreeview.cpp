#include "objecttreeview.h"
#include "lazynutobjectmodel.h"
#include "enumclasses.h"
#include "expandtofillbutton.h"


#include <QApplication>
#include <QHeaderView>
#include <QDesktopWidget>

ObjectTreeView::ObjectTreeView(QWidget *parent)
    : QTreeView(parent), objectModel(nullptr)
{
    header()->setStretchLastSection(true);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setSelectionMode(QAbstractItemView::SingleSelection);
    expandToFillButton = new ExpandToFillButton(this);
    setItemDelegateForColumn(1, expandToFillButton);
    connect(expandToFillButton, SIGNAL(expandToFill(QAbstractItemModel*,const QModelIndex&,QString)),
            this, SLOT(triggerFillList(QAbstractItemModel*,const QModelIndex&,QString)));

}

void ObjectTreeView::setModel(QAbstractItemModel *_objectModel)
{
    objectModel = qobject_cast<LazyNutObjectModel *>(_objectModel);
    if (objectModel)
    {
        QTreeView::setModel(objectModel);
        connect(this, SIGNAL(clicked(QModelIndex)), objectModel, SLOT(sendObjectRequested(QModelIndex)));
        connect(objectModel, &QAbstractItemModel::modelReset, [=]()
        {
            expandAll();
            for (int col=0; col < model()->columnCount(); ++col)
                resizeColumnToContents(col);
//            updateGeometry();
        });
    }
    else
        eNerror << "invalid model. Model should be of class LazyNutObjectModel";
}

QSize ObjectTreeView::minimumSizeHint() const
{
    return QSize(header()->length(), qApp->desktop()->screenGeometry().height()/2 );
}

void ObjectTreeView::doFillList(QDomDocument *dom)
{
    if (objectModel)
        objectModel->pokeAdditionalDescription(index_to_expand, dom);
}

void ObjectTreeView::triggerFillList(QAbstractItemModel *, const QModelIndex &ind, QString name)
{
    index_to_expand = ind;
    LazyNutJob *job = new LazyNutJob;
    job->cmdList = QStringList({QString("xml %1 description").arg(name)});
    job->setAnswerReceiver(this, SLOT(doFillList(QDomDocument*)), AnswerFormatterType::XML);
    SessionManager::instance()->submitJobs(job);
}
