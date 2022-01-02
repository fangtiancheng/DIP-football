#ifndef COMMON_H
#define COMMON_H
#include <QApplication>
#include <QWidget>
#include <QtWidgets>
#include <string>
#include <QDebug>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
//#include "jsoncpp/include/json/json.h"
#include <QMap>
class ConfigMessage: public QObject{
Q_OBJECT

public:
    QString dirName;
    QString videoPath;
    QString result1Path, result2Path;
    QString json1Path, json2Path;
    QString ip, port;
    ConfigMessage();
    bool save();
    bool load();
    QString toJson() const;
signals:
    void error (const QString& errMsg);
};

struct FootballPlayer{
    QStringList verb;
    QString skill;
    QString fight;
    FootballPlayer();
    FootballPlayer(const QJsonObject&);
};

void loadFrameJson(const QString&, QMap<int, QMap<int, FootballPlayer>>*);
void saveFrameJson(const QMap<int, QMap<int, FootballPlayer>>*, QString&);
#endif // COMMON_H
