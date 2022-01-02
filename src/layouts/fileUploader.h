// This class is modified according to the open source code of GitHub
// https://github.com/shengyu7697/QtFileTransfer
#ifndef FILEUPLOADER_H
#define FILEUPLOADER_H

#include <QDialog>
#include <QTextBrowser>
#include <QAbstractSocket>
#include <QTcpSocket>
#include <QFile>


class FileUploader: public QTcpSocket
{
    Q_OBJECT

public:
    explicit FileUploader(QWidget* parent, QTextEdit *logBrowser);
    explicit FileUploader(QWidget* parent);
    ~FileUploader();
    void setFileName(const QString&);
    void startTransfer();
protected:
    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *event);

private slots:
    bool openFile(QString &fileName);
    void send();
    void stop();
    void updateClientProgress(qint64);
    void displayError(QAbstractSocket::SocketError);

signals:
    void log(const QString&);
    void finish();
private:
    QTextEdit* logBrowser;
    void appendLog(const QString& text);

    QFile *localFile;
    qint64 totalBytes;
    qint64 bytesWritten;
    qint64 bytesToWrite;
    qint64 payloadSize;
    QString fileName;
    QByteArray outBlock;
};

#endif // FILEUPLOADER_H
