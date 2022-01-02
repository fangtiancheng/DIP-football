#ifndef FILEUPLOADER_H
#define FILEUPLOADER_H
#include <QAbstractSocket>
#include <QTcpSocket>
#include <QFile>
#include <QtNetwork>

class FileUploader: public QObject
{
    Q_OBJECT

public:
    explicit FileUploader(QTcpSocket* socket, QObject* parent);
    ~FileUploader();
    void setFileName(const QString&);
    void startTransfer();
    enum STATUS{
        UNUSED, USING, USED
    };
    STATUS getStatus();

private slots:
    bool openFile(QString &fileName);
    void stop();
    void updateFileUploaderProgress(qint64);
    void displayError(QAbstractSocket::SocketError);

signals:
    void log(const QString&);
    void finish();
private:
    void appendLog(const QString& text);
    QTcpSocket* socket;
    STATUS status;
    QFile *localFile;
    qint64 totalBytes;
    qint64 bytesWritten;
    qint64 bytesToWrite;
    qint64 payloadSize;
    QString fileName;
    QByteArray outBlock;
};

#endif // FILEUPLOADER_H
