#ifndef DATAFRAMEVIEWER_H
#define DATAFRAMEVIEWER_H

#include "dataviewer.h"

#include <QMap>
#include <QMultiMap>
#include <QTextDocument>

class DataFrameModel;
class ParametersProxyModel;
class QTableView;
class ObjectCacheFilter;
class ObjectUpdater;
class QDomDocument;
class FindDialog;
class QToolButton;

class DataframeViewer : public DataViewer
{
    Q_OBJECT
    friend class DataframeViewerDispatcher;
    Q_PROPERTY(bool dragDropColumns READ dragDropColumns WRITE setDragDropColumns NOTIFY dragDropColumnsChanged)
    Q_PROPERTY(bool stimulusSet READ stimulusSet WRITE setStimulusSet NOTIFY stimulusSetChanged)
    Q_PROPERTY(bool parametersTable READ parametersTable WRITE setParametersTable NOTIFY parametersTableChanged)
public:
    DataframeViewer(Ui_DataViewer *ui, QWidget * parent = 0);
    bool dragDropColumns() {return m_dragDropColumns;}
    void setDragDropColumns(bool enabled) {m_dragDropColumns = enabled; emit dragDropColumnsChanged(enabled);}
    bool stimulusSet() {return m_stimulusSet;}
    void setStimulusSet(bool isStimulusSet) {m_stimulusSet = isStimulusSet; emit stimulusSetChanged(isStimulusSet);}
    bool parametersTable() {return m_parametersTable;}
    void setParametersTable(bool isParametersTable);

public slots:
    virtual void dispatch() Q_DECL_OVERRIDE;
    virtual void open() Q_DECL_OVERRIDE;
    virtual void save() Q_DECL_OVERRIDE;
    virtual void copy() Q_DECL_OVERRIDE;
    void copyDataframe();
    virtual void addRequestedItem(QString name="", bool isBackup=false);
    virtual void snapshot(QString name="") Q_DECL_OVERRIDE {Q_UNUSED(name)}
    virtual void setCurrentItem(QString name) Q_DECL_OVERRIDE;


protected slots:
    virtual void destroyItem_impl(QString name) Q_DECL_OVERRIDE;
    virtual void enableActions(bool enable) Q_DECL_OVERRIDE;
    void updateDataframe(QDomDocument* domDoc, QString name);
    void askGetEntireDataframe();
    void getEntireDataframe();
    void showFindDialog();
    void findForward(const QString &str, QFlags<QTextDocument::FindFlag> flags);
    void setParameter(QString name, QString key_val);
    void sendNewPlotRequest();
    void sendNewDataframeViewRequest();

signals:
    void dragDropColumnsChanged(bool);
    void stimulusSetChanged(bool);
    void parametersTableChanged(bool);
    void createDataViewRequested(QString name, QString dataViewType, QString rScript, QMap<QString, QString> defaultSettings, bool isBackup = false);

protected:
    virtual void addItem_impl(QString name) Q_DECL_OVERRIDE;
    virtual QWidget *makeView(QString name);
    virtual void addNameToFilter(QString name);
    virtual void removeNameFromFilter(QString name);
    virtual void setNameInFilter(QString name);
    void addExtraActions();
    bool partiallyLoaded(QString name = "");
    bool dataframeExceedsCellLimit(QString name, int maxCells);
    void limitedGet(QString name, int maxCells);
    void doCopy();
    void sendNewDataViewRequest(QAction *action, QString subtype);
    QString reference(QString name);



    QMap<QString, DataFrameModel*> modelMap;
    QMap<QString, ParametersProxyModel*> referenceProxyMap; // only if parametersTable()
    ObjectCacheFilter *dataframeFilter;
    ObjectUpdater *dataframeUpdater;
    QStringList requestedDataframeViews;
    bool m_dragDropColumns;
    bool m_stimulusSet;
    bool m_parametersTable;
    FindDialog*     findDialog;
    QAction *getAllAct;
    QAction *findAct;
    QAction *copyDFAct;
    QToolButton *plotButton;
    QToolButton *dataframeViewButton;
    int maxRows;
    int maxCols;
    int maxFirstDisplayCells;
    int maxDisplayCells;
};

#endif // DATAFRAMEVIEWER_H
