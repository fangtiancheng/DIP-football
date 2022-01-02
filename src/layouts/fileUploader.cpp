#include "fileUploader.h"
#include <QtNetwork>
#include <QFileDialog>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>

FileUploader::FileUploader(QWidget* parent) :
    QTcpSocket(parent)
{
    payloadSize = 64*1024; // 64KB
    totalBytes = 0;
    bytesWritten = 0;
    bytesToWrite = 0;


    connect(this, SIGNAL(bytesWritten(qint64)),
            this, SLOT(updateClientProgress(qint64)));

}

FileUploader::~FileUploader()
{

}

bool FileUploader::openFile(QString &fileName)
{
    if (fileName.isEmpty())
        return false;
    appendLog(tr("begin to upload %1").arg(fileName));
    return true;
}

void FileUploader::send()
{
    // initialize
    bytesWritten = 0;
    appendLog(tr("connecting..."));
}

void FileUploader::stop()
{
    this->close();
    appendLog(tr("stop connection!"));
}

void FileUploader::startTransfer()
{
    localFile = new QFile(fileName);
    if (!localFile->open(QFile::ReadOnly)) {
        appendLog("fileUploader: open file error!");
        return;
    }
    // get file size
    totalBytes = localFile->size();

    QDataStream sendOut(&outBlock, QIODevice::WriteOnly);
    sendOut.setVersion(QDataStream::Qt_4_0);
    QString currentFileName = fileName.right(fileName.size() - fileName.lastIndexOf('/')-1);
    // Reserve the total size information space, file name size information space, and then enter the file name
    sendOut << qint64(0) << qint64(0) << currentFileName;

    // The total size here is the sum of the total size information, file name size information, file name and actual file size
    totalBytes += outBlock.size();
    sendOut.device()->seek(0);

    // Return to the beginning of outbolock and replace the two qint64(0) spaces with the actual size information
    sendOut << totalBytes << qint64((outBlock.size() - sizeof(qint64)*2));

    // The size of the remaining data after sending the file header structure
    bytesToWrite = totalBytes - this->write(outBlock);

    outBlock.resize(0);
}

void FileUploader::updateClientProgress(qint64 numBytes)
{
    // 已經發送數據的大小
    bytesWritten += (int)numBytes;

    // 如果已經發送了數據
    if (bytesToWrite > 0) {
        // 每次發送payloadSize大小的數據，這裡設置為64KB，如果剩餘的數據不足64KB，
        // 就發送剩餘數據的大小
        outBlock = localFile->read(qMin(bytesToWrite, payloadSize));

        // 發送完一次數據後還剩餘數據的大小
        bytesToWrite -= (int)this->write(outBlock);

        // 清空發送緩衝區
        outBlock.resize(0);
    } else { // 如果沒有發送任何數據，則關閉文件
        localFile->close();
    }
    // 更新進度條


    // 如果發送完畢
    if (bytesWritten == totalBytes) {
        appendLog(QStringLiteral("upload file %1 success!").arg(fileName));
        localFile->close();
        // this->close();
        emit finish();
    }
}

void FileUploader::displayError(QAbstractSocket::SocketError)
{
    appendLog("Error: " + this->errorString());
    stop();
}
void FileUploader::setFileName(const QString& fileName){
    this->fileName = fileName;
}


void FileUploader::dragEnterEvent(QDragEnterEvent *event)
{
    qDebug() << "dragEnterEvent";
    if (event->mimeData()->hasFormat("text/uri-list")) {
        event->acceptProposedAction();
    }
}

void FileUploader::dropEvent(QDropEvent *event)
{
    qDebug() << "dropEvent";


    QList<QUrl> urls = event->mimeData()->urls();
    if (urls.isEmpty()) {
        appendLog("Error: urls is empty");
        return;
    }

    fileName = urls.first().toLocalFile();
    if (!openFile(fileName)) {
        appendLog("Error: openFile failed");
        return;
    }
}

void FileUploader::appendLog(const QString& text)
{
    qDebug() << text;
    emit log(text);
}
