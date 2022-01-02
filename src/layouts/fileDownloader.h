// This class is modified according to the open source code of GitHub
// https://github.com/shengyu7697/QtFileTransfer
#ifndef FILEDOWNLOADER_H
#define FILEDOWNLOADER_H
#include <QTcpSocket>
#include <QtNetwork>
#include <QTextStream>
#include <iostream>
class QTcpSocket;
class QFile;


class FileDownloader: public QObject
{
    Q_OBJECT

public:
    explicit FileDownloader(QTcpSocket* soc, QObject* father, const QString& downloadLocation);
    ~FileDownloader();
    void setDownloadLocation(const QString& loc);
signals:
    void finish(QString);
    void error();
    void log(QString);
private slots:
    void displayError();
    // for downloader
    void updateServerProgress();


private:
    QTcpSocket* socket = nullptr;
    QTextStream cin, cout, cerr;
    // for downloader
    qint64 downloadTotalBytes = 0;
    qint64 bytesReceived = 0;
    qint64 fileNameSize = 0;
    QString downloadFileName;
    QFile *downloadFile = nullptr;
    QByteArray inBlock;
    QString downloadLocation;
};

#endif // FILEDOWNLOADER_H
