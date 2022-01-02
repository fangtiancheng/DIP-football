#include "common.h"
ConfigMessage::ConfigMessage(){

}

bool ConfigMessage::save(){
    QString errMsg;
    const QString configFilePath = this->dirName + "/config.json";
    QSaveFile configFile(configFilePath);
    if(configFile.open(QFile::WriteOnly | QFile::Text)){
        QString json = this->toJson();
        QTextStream out(&configFile);
        out << json;
        if(!configFile.commit()){
            errMsg = tr("Cannot Write file '%1'").arg( configFilePath );
        }
    }
    else{
        errMsg = tr("Cannot save config file to '%1'").arg( configFilePath );
    }
    if(!errMsg.isEmpty()){
        emit error(errMsg);
        return false;
    }
    return true;
}

bool ConfigMessage::load(){
    const QString configFilePath = this->dirName + "/config.json";
    QFile jsonFile(configFilePath);
    if(!jsonFile.open(QFile::ReadOnly | QFile::Text)){
        emit error("open file " + configFilePath + " error");
        return false;
    }
    QJsonParseError* jsonError = new QJsonParseError;
    QJsonDocument json = QJsonDocument::fromJson(jsonFile.readAll(), jsonError);
    if(jsonError->error != QJsonParseError::NoError){
        qDebug() << "parse json error: " << jsonError->errorString();
        emit error("parse json error: " + jsonError->errorString());
        delete jsonError;
        return false;
    }
    QJsonObject root = json.object();
    this->videoPath = root["videoPath"].toString();
    this->result1Path = root["result1Path"].toString();
    this->result2Path = root["result2Path"].toString();
    this->json1Path = root["json1Path"].toString();
    this->json2Path = root["json2Path"].toString();
    this->ip = root["ip"].toString();
    this->port = root["port"].toString();
    return true;
}

QString ConfigMessage::toJson() const{
    QJsonObject root;
    root["videoPath"] = this->videoPath;
    root["result1Path"] = this->result1Path;
    root["result2Path"] = this->result2Path;
    root["json1Path"] = this->json1Path;
    root["json2Path"] = this->json2Path;
    root["ip"] = this->ip;
    root["port"] = this->port;
    return QString(QJsonDocument(root).toJson());
}

FootballPlayer::FootballPlayer(const QJsonObject& playerJson){
    this->skill = playerJson["skill"].toString();
    this->fight = playerJson["fight"].toString();
    QJsonArray players = playerJson["verb"].toArray();
    for(auto playerIter=players.cbegin(); playerIter!=players.cend();playerIter++){
        this->verb.append(playerIter->toString());
    }
}
FootballPlayer::FootballPlayer(){}
void loadFrameJson(const QString& fileName, QMap<int, QMap<int, FootballPlayer>>* frames)
{
    QFile jsonFile(fileName);
    if(!jsonFile.open(QFile::ReadOnly | QFile::Text)){
        throw std::runtime_error("open file " + fileName.toStdString() + " error");
    }
    QJsonParseError* error = new QJsonParseError;
    QJsonDocument json = QJsonDocument::fromJson(jsonFile.readAll(), error);
    if(error->error != QJsonParseError::NoError){
        qDebug() << "parse json error: " << error->errorString();
        throw std::runtime_error(error->errorString().toStdString());
    }
    QJsonObject f = json.object();
    QStringList frameIdxs = f.keys();
    foreach(QString frameIdx, frameIdxs){
        int frameId = frameIdx.toInt();
        QJsonObject players = f[frameIdx].toObject();
        QStringList playerIdxs = players.keys();
        QMap<int, FootballPlayer> tmp;
        foreach(QString playerIdx, playerIdxs){
            int playerId = playerIdx.toInt();
            QJsonObject player = players[playerIdx].toObject();
            tmp[playerId] = FootballPlayer(player);
        }
        (*frames)[frameId] = tmp;
    }
    delete error;
}
void saveFrameJson(const QMap<int, QMap<int, FootballPlayer>>* frames, QString&fileName){
    QFile jsonFile(fileName);
    if(!jsonFile.open(QFile::WriteOnly)){
        throw std::runtime_error("open file " + fileName.toStdString() + " error");
    }
    QJsonDocument json;
    QJsonObject jsonFrames;
    for(auto frameIter=frames->cbegin();frameIter!=frames->cend();frameIter++){
        QString frameIndex =QString::number(frameIter.key());
        const QMap<int, FootballPlayer>& frame = frameIter.value();
        QJsonObject jsonFrame;
        for(auto playerIter=frame.cbegin();playerIter!=frame.cend();playerIter++){
            const QString playerNum = QString::number(playerIter.key());
            const FootballPlayer& player = playerIter.value();
            QJsonObject jsonPlayer;
            QJsonArray jsonVerb;
            for(auto verbIter=player.verb.cbegin();verbIter!=player.verb.cend();verbIter++){
                jsonVerb.append(*verbIter);
            }
            jsonPlayer["verb"] = jsonVerb;
            jsonPlayer["skill"] = player.skill;
            jsonPlayer["fight"] = player.fight;
            jsonFrame[playerNum] = jsonPlayer;
        }
        jsonFrames[frameIndex] = jsonFrame;
    }
    json.setObject(jsonFrames);
    jsonFile.write(json.toJson(QJsonDocument::Indented));
    jsonFile.close();
}
