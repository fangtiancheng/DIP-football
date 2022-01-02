#ifndef SERVER_H
#define SERVER_H
#include "connection.h"
#include "fileDownloader.h"
#include "fileUploader.h"

// ****************************************
class TcpServer: public QTcpServer{
    Q_OBJECT
public:
    TcpServer(QObject* parent): QTcpServer(parent){
        listen(QHostAddress::Any, 4399);
    }

signals:
    void newConnection(qintptr socketDescriptor);

protected:
    void incomingConnection(qintptr socketDescriptor) override {
        emit newConnection(socketDescriptor);
    }
};
// ****************************************
class Server: public QObject{
    Q_OBJECT
private:
    // connect a client
//    QList<Connection*> connectionList;
    Connection* connection;
    QList<FileDownloader*> fileDownloaderList;
    QList<FileUploader*> fileUploaderList;
    int fileStatus;
    // 0bxx00: forbid file transfer;
    // 0bxx01: next is upload from client to server
    // 0bxx10: next is download from server to client
    // 0b0001: no move
    // 0b0101: mv ./demo.mp4 ./inputfile/demo.mp4
    // 0b1001: mv ./human_trans.pkl ./trans_out/human_trans.pkl
    QString downloadFileName;
    void receiveDownloadRequest();
    void receiveUploadRequest();
    void exePython();
    TcpServer* server;
    QProcess* cmd;
    QTextStream cin, cout, cerr;
    QHash<QString, QString> downloadFileMap;
private slots:
    void receiveMessage(const QString& from, const QString& message);
    void downloadFinish();
    void downloadError();
    void cmdReadOutput();
    void cmdReadError();
    void receiveConnection(qintptr socketDescriptor);
    void closeConnection();
    void appendLog(const QString&);
    void processDataFinish();

public:
    Server(QObject* father);
    ~Server();
};

#endif // SERVER_H
