#include <QDomDocument>
#include <QtWidgets>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListView>
#include <QItemSelectionModel>
#include <QModelIndex>
#include <QtSql>
#include <QStandardItemModel>
#include <QMimeData>

#include "tableviewer2.h"
#include "dataframemodel.h"
#include "objectcatalogue.h"
#include "objectcataloguefilter.h"
#include "lazynutjobparam.h"
#include "lazynutjob.h"
#include "sessionmanager.h"
#include "finddialog.h"

TableViewer::TableViewer(const QString &tableName, QWidget *parent)
    : QMainWindow(parent)
{
    // this type of form is used for stimSetForm and results tables

    createActions();
    createToolBars();

    tablePanel = new QTabWidget;
    numTables = 0;
    currentTableIdx = 0;
//    widget = new QWidget(this);
//    setCentralWidget(widget);
    setCentralWidget(tablePanel);
//    QVBoxLayout* layout = new QVBoxLayout(widget);
//    layout->addWidget(tableBox);
//    layout->addWidget(view);
//    widget->setLayout(layout);
//    addTable("bla");
}

int TableViewer::addTable(QString name)
{
    tables.push_back(new QTableView(this));
    myHeaders.push_back(new DataFrameHeader(tables.back()));
    numTables++;
    currentTableIdx = numTables-1;
    int idx = tablePanel->addTab(tables[numTables-1], tr("Table ")+QString::number(numTables));
    qDebug() << "adding table to panel. New numTables = " << numTables;
    tablePanel->setCurrentIndex(idx);
    tableMap[idx] = name;

    tables[tablePanel->currentIndex()]->setEditTriggers(QAbstractItemView::NoEditTriggers);
    // stimulus set allows a column to be dragged
    tables[tablePanel->currentIndex()]->setSelectionMode(QAbstractItemView::ExtendedSelection);
    tables[tablePanel->currentIndex()]->setDragEnabled(true);
    tables[tablePanel->currentIndex()]->setDropIndicatorShown(true);
    tables[tablePanel->currentIndex()]->setDragDropMode(QAbstractItemView::DragOnly);

    tables[tablePanel->currentIndex()]->setHorizontalHeader(myHeaders[tablePanel->currentIndex()]);
    connect(myHeaders[tablePanel->currentIndex()], SIGNAL(columnDropped(QString)), this, SIGNAL(columnDropped(QString)));
    connect(myHeaders[tablePanel->currentIndex()], SIGNAL(restoreComboBoxText()), this, SIGNAL(restoreComboBoxText()));
    connect(this,SIGNAL(newTableName(QString)),myHeaders[tablePanel->currentIndex()],SLOT(setTableName(QString)));

    return(numTables);
}

void TableViewer::setTableText(QString text)
{
    tableBox->addItem(text);
    tableBox->setCurrentIndex(tableBox->findData(text,Qt::DisplayRole));
    emit newTableName(text);
}

void TableViewer::createActions()
{
    openAct = new QAction(QIcon(":/images/open.png"), tr("&Open..."), this);
    openAct->setShortcuts(QKeySequence::Open);
    openAct->setStatusTip(tr("Open an existing file"));
    connect(openAct, SIGNAL(triggered()), this, SIGNAL(openFileRequest()));


    saveAct = new QAction(QIcon(":/images/save.png"), tr("&Save"), this);
    saveAct->setShortcuts(QKeySequence::Save);
    saveAct->setStatusTip(tr("Save the document to disk"));
    connect(saveAct, SIGNAL(triggered()), this, SLOT(save()));

    copyAct = new QAction(QIcon(":/images/clipboard.png"), tr("&Copy to clipboard"), this);
    copyAct->setShortcuts(QKeySequence::Copy);
    copyAct->setStatusTip(tr("Copy the current selection's contents to the "
                             "clipboard"));
    connect(copyAct, SIGNAL(triggered()), this, SLOT(on_copy_clicked()));
    copyAct->setEnabled(true);

    copyDFAct = new QAction(QIcon(":/images/copy.png"), tr("&Copy to new dataframe"), this);
    copyDFAct->setStatusTip(tr("Copy contents to a new dataframe "));
    connect(copyDFAct, SIGNAL(triggered()), this, SLOT(on_copy_DF_clicked()));
    copyDFAct->setEnabled(true);

    findAct = new QAction(QIcon(":/images/magnifying-glass-2x.png"), tr("&Find"), this);
    findAct->setShortcuts(QKeySequence::Find);
    findAct->setToolTip(tr("Find text in this table"));
    connect(findAct, SIGNAL(triggered()), this, SLOT(showFindDialog()));
    findAct->setEnabled(true);

}

void TableViewer::save()
{
    QSettings settings("QtEasyNet", "nmConsole");
    QString workingDir = settings.value("easyNetHome").toString();
    QString currFilename = workingDir + "/Output_files/.";
    qDebug() << "df currFilename" << currFilename;
    QString fileName = QFileDialog::getSaveFileName(this,
                        tr("Choose a file name"), currFilename,
                        tr("CSV (*.csv)"));
    if (fileName.isEmpty())
        return;

    QString cmd = currentTable + " save_csv " + fileName;
    SessionManager::instance()->runCmd(cmd);

}

void TableViewer::createToolBars()
{
    fileToolBar = addToolBar(tr("File"));
    if (openAct)
        fileToolBar->addAction(openAct);
    fileToolBar->addAction(saveAct);

    editToolBar = addToolBar(tr("Edit"));
    editToolBar->addAction(copyDFAct);
    editToolBar->addAction(findAct);

    fileToolBar->setMovable(false);
    editToolBar->setMovable(false);
}

void TableViewer::setView(QString name)
{
    qDebug() << "Entered setView";
    emit newTableSelection(name);
}

void TableViewer::addDataFrameToWidget(QDomDocument* domDoc)
{
    dfModel = new DataFrameModel(domDoc, this); // you only need this line to load in the entire XML table
    connect(dfModel, SIGNAL(newParamValueSig(QString)),
            this,SIGNAL(newParamValueSig(QString)));

    tables[tablePanel->currentIndex()]->setModel(dfModel);
    tables[tablePanel->currentIndex()]->resizeColumnsToContents();
    tables[tablePanel->currentIndex()]->show();
    // at this point we have a view widget showing the table
    tables[tablePanel->currentIndex()]->verticalHeader()->hide(); // hideColumn(0); // 1st column contains rownames, which user doesn't need
    QItemSelectionModel* selModel = tables[tablePanel->currentIndex()]->selectionModel();
    connect(selModel, SIGNAL(currentRowChanged(QModelIndex, QModelIndex)),
            this,SLOT(rowChangedSlot( const QModelIndex& , const QModelIndex& )));
}

void TableViewer::replaceHeaders(QTableView* view)
{
    QStringList headers, newHeaders;
    for(int i = 0; i < view->model()->columnCount(); i++)
        headers.append(view->model()->headerData(i, Qt::Horizontal).toString());

    qDebug() << "Here are the original headers" << headers;

    QString trial = "ldt"; // for example
    int col=0;
    foreach(QString header,headers)
    {
        header.replace("(","");
        header.replace(")","");
        header.replace("event_pattern","");
        header.replace(trial,"");
        newHeaders.append(header.simplified());
        QVariant qv(header.simplified());

        /* DOESN'T WORK */
        view->model()->setHeaderData(col, Qt::Horizontal,qv);
        col++;

    }
    qDebug() << "Here are the replaced headers" << newHeaders;

}



void TableViewer::rowChangedSlot( const QModelIndex& selected, const QModelIndex& deselected )
{
    QString key = (tables[tablePanel->currentIndex()]->model()->data(selected.sibling(selected.row(), 0) ,0)).toString();
//    qDebug() << key;
    emit currentKeyChanged(key);

}

void TableViewer::updateParamTable(QString model)
{
    if (model.isEmpty())
        return;
    qDebug() << "Entered updateParamTable():" << model;
    QString name = QString("(") + model + QString(" parameters)");
    emit setParamDataFrameSignal(name);
    updateTableView(name);
}

void TableViewer::resizeColumns()
{
    tables[tablePanel->currentIndex()]->resizeColumnsToContents();

//    for (int i=0; i<view->columns();i++)
//        setColumnWidth(i, .view->width() / columns());
}

void TableViewer::on_copy_clicked()
{
    QAbstractItemModel *abmodel = tables[tablePanel->currentIndex()]->model();
    QItemSelectionModel *model = tables[tablePanel->currentIndex()]->selectionModel();
    QModelIndexList list = model->selectedIndexes();

    if(list.size() < 2)
    {
//         select all
                list.clear();
                for(int i=0;i<abmodel->rowCount();i++)
                    for(int j=0;j<abmodel->columnCount();j++)
                        list.append(abmodel->index(i,j));
    }
    qSort(list);
    qDebug() << list;

    QString copy_table;
    QModelIndex last = list.last();
    QModelIndex previous = list.first();

    list.removeFirst();

    for(int i = 0; i < list.size(); i++)
    {
        QVariant data = abmodel->data(previous);
        QString text = data.toString();

        QModelIndex index = list.at(i);
        copy_table.append(text);

        if(index.row() != previous.row())

        {
            copy_table.append('\n');
        }
        else
        {
            copy_table.append('\t');
        }
        previous = index;
    }

    copy_table.append(abmodel->data(list.last()).toString());
    copy_table.append('\n');

    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(copy_table);

}

void TableViewer::on_copy_DF_clicked()
{
    QString cmd = currentTable + " copy " + "new_df";
    SessionManager::instance()->runCmd(cmd);

}

void TableViewer::dragEnterEvent(QDragEnterEvent *event)
{
    qDebug() << "drageenterevent";
    if (event->mimeData()->hasFormat("application/x-dnditemdata"))
    {
        if (event->source() == this)
        {
            event->setDropAction(Qt::MoveAction);
            event->accept();
        }
        else
        {
            event->acceptProposedAction();
        }
    }
    else
    {
        event->ignore();
    }
}

void TableViewer::dragMoveEvent(QDragMoveEvent *event)
{
//if (event->mimeData()->hasFormat("application/x-dnditemdata"))
{
    if (event->source() == this) {
        event->setDropAction(Qt::MoveAction);
        event->accept();
    } else {
        event->acceptProposedAction();
    }
}
//else {
//    event->ignore();
//}
}

//! [1]
void TableViewer::mousePressEvent(QMouseEvent *event)
{
QTableView *child = static_cast<QTableView*>(childAt(event->pos()));
if (!child)
    return;

qDebug("child name %s", child->objectName().toLatin1().data());

//if (!child->text().isEmpty())
//{
//    draggedText = child->text();
//    qDebug() << "draggedText" << draggedText;
//}

QByteArray itemData;
QDataStream dataStream(&itemData, QIODevice::WriteOnly);
//dataStream << pixmap << QPoint(event->pos() - child->pos());
//! [1]

//! [2]
QMimeData *mimeData = new QMimeData;
mimeData->setData("application/x-dnditemdata", itemData);
//! [2]

//! [3]
QDrag *drag = new QDrag(this);
drag->setMimeData(mimeData);
drag->setHotSpot(event->pos() - child->pos());
//! [3]


if (drag->exec(Qt::CopyAction | Qt::MoveAction, Qt::CopyAction) == Qt::MoveAction)
    child->close();
else
    child->show();
}

bool TableViewer::dropMimeData(const QMimeData * data, Qt::DropAction action, int row, int column, const QModelIndex & parent)
{
  QStringList formats = data->formats();
  QByteArray encodedData = data->data(formats[0]);
  QDataStream stream(&encodedData, QIODevice::ReadOnly);

//  int row, column;
  stream >> row >> column;

  qDebug() << "row: " << row << " column:" << column;

  return false;
}

void TableViewer::updateTableView(QString text)
{
//    qDebug() << this << "Entered updateTableView with " << text;
//    qDebug() << "currentIndex = " << tables[tablePanel->currentIndex()]->currentIndex();
    if (!text.size())
        return;
    if (text=="Untitled")
        return;
    if (text==currentTable)
        return;

    currentTable = text;
    LazyNutJobParam *param = new LazyNutJobParam;
    param->logMode &= ECHO_INTERPRETER;
    param->cmdList = QStringList({QString("xml " + text + " get")});
    param->answerFormatterType = AnswerFormatterType::XML;
    param->setAnswerReceiver(this, SLOT(addDataFrameToWidget(QDomDocument*)));
    SessionManager::instance()->setupJob(param, sender());
}


void TableViewer::showFindDialog()
{
    findDialog = new FindDialog;
    findDialog->hideExtendedOptions();
    connect(findDialog, SIGNAL(findForward(QString, QFlags<QTextDocument::FindFlag>)),
            this, SLOT(findForward(QString, QFlags<QTextDocument::FindFlag>)));

    findDialog->show();
    findDialog->raise();
    findDialog->activateWindow();
}

void TableViewer::findForward(const QString &str, QFlags<QTextDocument::FindFlag> flags)
{
    QFlags<Qt::MatchFlag> flag;
//    if (flags |= QTextDocument::FindCaseSensitively)
//        flag = Qt::MatchCaseSensitive;
//    else
        flag = Qt::MatchExactly;
    QVariant qv(str);

    // first try searching in the current column
    int row = tables[tablePanel->currentIndex()]->currentIndex().row();
    int col = tables[tablePanel->currentIndex()]->currentIndex().column();
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
        tables[tablePanel->currentIndex()]->setCurrentIndex(hits.first());
//        findDialog->hide();
    }
    else
        QMessageBox::warning(this, "Find",QString("The text was not found"));
//        findDialog->hide();

}


