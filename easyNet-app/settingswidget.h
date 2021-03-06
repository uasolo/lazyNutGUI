#ifndef SETTINGSWIDGET_H
#define SETTINGSWIDGET_H


#include <QWidget>
#include <QMap>
#include <QSharedPointer>


class QDomDocument;
class QLabel;
class QLineEdit;
class QPushButton;
class QScrollArea;
class PlotSettingsForm;
class QGridLayout;
class QToolButton;
class ObjectCacheFilter;
class ObjectUpdater;


class SettingsWidget : public QWidget
{
    Q_OBJECT
public:
    SettingsWidget(QWidget *parent = 0);
    ~SettingsWidget();
    void setSetting(QString setting, QString value);
    QMap<QString, QString> getSettings(QString name);

public slots:
    void sendSettings(QString name = QString(), bool force = false);
    void newForm(QString name, QString dataViewType, QString rScript, QMap <QString,QString> defaultSettings, bool isBackup = false); // old newRPlot
    void setForm(QString name); // old setPlotSettings
    void createNewForm(QString name="");

signals:
    void dataViewCreated(QString, bool); // old newRPlotCreated
    void settingsApplied(QString);
    void closeRequested();


protected slots:
    void buildSettingsForm(QString name, QDomDocument *domDoc,
                           QMap<QString, QString> defaultSettings=QMap<QString, QString>());
    void buildSettingsForm();
    void rebuildForm();
    void apply();
    void reloadScript();
    void setCurrentSettings(QDomDocument *settingsList) {currentSettings = settingsList;}
    void setCurrentPlotName(QString name) {currentName = name;}
    void removeForm(QString name);
    void emitDataViewCreated();

protected:
    void buildWidget();
    void clearForm();
    QString type(QString name);


    QString currentName;
    QMap<QString, PlotSettingsForm*> formMap;
    QMap<QString, QString> typeMap;
    QDomDocument *currentSettings;
    ObjectCacheFilter *descriptionFilter;
    ObjectUpdater *descriptionUpdater;
    QPushButton *reloadScriptButton;
    QPushButton *applyButton;
    QPushButton *okButton;
    QLineEdit *nameEdit;
    QLineEdit *typeEdit;
    QScrollArea *formScrollArea;
};

#endif // SETTINGSWIDGET_H
