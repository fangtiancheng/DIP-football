#include "fileUploader.h"
#define DEBUG qDebug() << "in file " << __FILE__ << ", line " << __LINE__;

FileUploader::FileUploader(QTcpSocket* socket, QObject* parent) :
    QObject(parent), socket(socket)
{

    payloadSize = 64*1024; // 64KB
    totalBytes = 0;
    bytesWritten = 0;
    bytesToWrite = 0;
    this->status = UNUSED;

    connect(socket, SIGNAL(bytesWritten(qint64)),
            this, SLOT(updateFileUploaderProgress(qint64)));
}

FileUploader::~FileUploader()
{
    delete localFile;
}

bool FileUploader::openFile(QString &fileName)
{
    if (fileName.isEmpty())
        return false;
    appendLog(tr("begin to upload %1").arg(fileName));
    return true;
}

//void FileUploader::send()
//{
//    // 初始化已發送字節為0
//    bytesWritten = 0;
//    appendLog(tr("connecting..."));
//}

void FileUploader::stop()
{
    socket->close();
    appendLog(tr("stop connection!"));
}

void FileUploader::startTransfer()
{
    localFile = new QFile(fileName);
    if (!localFile->open(QFile::ReadOnly)) {
        appendLog(tr("FileUploader: open file %1 error!").arg(fileName));
        return;
    }
    // 獲取文件大小
    totalBytes = localFile->size();

    QDataStream sendOut(&outBlock, QIODevice::WriteOnly);
    sendOut.setVersion(QDataStream::Qt_4_0);
    QString currentFileName = fileName.right(fileName.size() - fileName.lastIndexOf('/')-1);
    // 保留總大小信息空間、文件名大小信息空間，然後輸入文件名
    sendOut << qint64(0) << qint64(0) << currentFileName;

    // 這裡的總大小是總大小信息、文件名大小信息、文件名和實際文件大小的總和
    totalBytes += outBlock.size();
    sendOut.device()->seek(0);

    // 返回outBolock的開始，用實際的大小信息代替兩個qint64(0)空間
    sendOut << totalBytes << qint64((outBlock.size() - sizeof(qint64)*2));

    // 發送完文件頭結構後剩餘數據的大小
    bytesToWrite = totalBytes - socket->write(outBlock);
    this->status = USING;
    appendLog(QStringLiteral("已連接"));
    outBlock.resize(0);
}

void FileUploader::updateFileUploaderProgress(qint64 numBytes)
{
    // 已經發送數據的大小
    bytesWritten += (int)numBytes;

    // 如果已經發送了數據
    if (bytesToWrite > 0) {
        // 每次發送payloadSize大小的數據，這裡設置為64KB，如果剩餘的數據不足64KB，
        // 就發送剩餘數據的大小
        outBlock = localFile->read(qMin(bytesToWrite, payloadSize));

        // 發送完一次數據後還剩餘數據的大小
        bytesToWrite -= (int)socket->write(outBlock);

        // 清空發送緩衝區
        outBlock.resize(0);
    } else { // 如果沒有發送任何數據，則關閉文件
        localFile->close();
    }
    // 更新進度條


    // 如果發送完畢
    if (bytesWritten == totalBytes) {
        appendLog(QStringLiteral("傳送文件 %1 成功").arg(fileName));
        localFile->close();
        socket->close();
        this->status = USED;
        emit finish();
    }
}

void FileUploader::displayError(QAbstractSocket::SocketError)
{
    appendLog("Error: " + socket->errorString());
    stop();
}
void FileUploader::setFileName(const QString& fileName){
    this->fileName = fileName;
}

FileUploader::STATUS FileUploader::getStatus(){
    return this->status;
}

void FileUploader::appendLog(const QString& text)
{
    qDebug() << text;
    emit log(text);
}
