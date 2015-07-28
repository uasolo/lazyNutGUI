#ifndef TABLEEDITOR_H
#define TABLEEDITOR_H

#include <QDialog>
#include <QMainWindow>

QT_BEGIN_NAMESPACE
class QToolBar;
class QDialogButtonBox;
class QPushButton;
class QSqlTableModel;
class QDomDocument;
class QLabel;
class QTableView;
class QVBoxLayout;
class QHBoxLayout;
class QListView;
class QModelIndex;
class QStandardItemModel;
class QMimeData;
class QComboBox;

class ObjectCatalogue;
class DataFrameModel;
class ObjectCatalogueFilter;

QT_END_NAMESPACE


class TableEditor : public QMainWindow
{
    Q_OBJECT

public:
    explicit TableEditor(ObjectCatalogue *objectCatalogue, const QString &tableName, QWidget *parent = 0);
    explicit TableEditor(const QString &tableName, QWidget *parent = 0);
    void setFilter(QString type);
    QStringList mimeTypes() const;
    QMimeData *mimeData; // (const QModelIndexList &indexes) const;

public slots:
    void setTableText(QString text);
signals:
    void currentKeyChanged(QString key);
    void newTableSelection(QString name);
    void setParamDataFrameSignal(QString);
    void newParamValueSig(QString);
    void columnDropped(QString set);
    void restoreComboBoxText();
    void newTableName(QString);
    void openFileRequest();

private slots:
//    void submit();
    void addDataFrameToWidget(QDomDocument* domDoc);
    void addRowToTable(QString cmd, QString time);
    void rowChangedSlot( const QModelIndex& selected, const QModelIndex& deselected );
    void updateParamTable(QString model);
    void resizeColumns();
    void setView(QString name);

    void on_copy_clicked();
    void updateTableView(QString text);
private:

    QToolBar *fileToolBar;
    QToolBar *editToolBar;
    QAction *openAct;
    QAction *saveAct;
    QAction *saveAsAct;
    QAction *copyAct;
    QAction *findAct;

    ObjectCatalogue  *objectCatalogue;
    ObjectCatalogueFilter *objectListFilter;
    QListView *objectListView;

    QPushButton *submitButton;
    QPushButton *revertButton;
    QPushButton *quitButton;
    QDialogButtonBox *buttonBox;
//    QSqlTableModel *model;
    QStandardItemModel *model;
    DataFrameModel  *dfModel;
    QTableView      *view;
    QComboBox       *tableBox;
    QLabel          *listTitle;
    QStringList     list;
    QWidget         *widget;
    QString         currentTable;

    void createToolBars();
    void createActions();
    void init(const QString &tableName, QWidget *parent);
    void setViewToStringList();
    void dragEnterEvent(QDragEnterEvent *event);
    void dragMoveEvent(QDragMoveEvent *event);
    void mousePressEvent(QMouseEvent *event);

    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent);
};


#endif