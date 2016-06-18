#ifndef DATAFRAMEVIEWERDISPATCHER_H
#define DATAFRAMEVIEWERDISPATCHER_H

#include "dataviewerdispatcher.h"

#include <QObject>

class DataframeViewer;

class DataframeViewerDispatcher : public DataViewerDispatcher
{
    Q_OBJECT
public:
    DataframeViewerDispatcher(DataframeViewer *host);
    virtual ~DataframeViewerDispatcher();
    virtual void preDispatch(QSharedPointer<QDomDocument> info) Q_DECL_OVERRIDE;
    virtual void dispatch(QSharedPointer<QDomDocument> info) Q_DECL_OVERRIDE;

protected:
    virtual QDomDocument *makePreferencesDomDoc() Q_DECL_OVERRIDE;

private:
    DataframeViewer *host;
    QMap<QString, int> previousDispatchModeMap;

};

#endif // DATAFRAMEVIEWERDISPATCHER_H
