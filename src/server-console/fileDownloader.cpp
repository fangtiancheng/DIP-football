#include "fileDownloader.h"
#define DEBUG qDebug() << "in file " << __FILE__ << ", line " << __LINE__;
QString getIpAddr()
{
    QString ipAddr;

    QList<QHostAddress> ipAddressesList = QNetworkInterface::allAddresses();
    // use the first non-localhost IPv4 address
    for (int i = 0; i < ipAddressesList.size(); ++i) {
        if (ipAddressesList.at(i) != QHostAddress::LocalHost &&
            ipAddressesList.at(i).toIPv4Address()) {
            ipAddr = ipAddressesList.at(i).toString();
            break;
        }
    }

    // if we did not find one, use IPv4 localhost
    if (ipAddr.isEmpty())
        ipAddr = QHostAddress(QHostAddress::LocalHost).toString();

    return ipAddr; // 獲取本機正在使用的IP地址
}

FileDownloader::FileDownloader(QTcpSocket* soc, QObject* father, const QString& downloadLocation = "./"):
    QObject(father), socket(soc), cin(stdin, QIODevice::ReadOnly),
    cout(stdout, QIODevice::WriteOnly), cerr(stderr, QIODevice::WriteOnly)
{
    QString ipAddr = getIpAddr();
    cout << tr("device ip: %1\n").arg(ipAddr);
    this->downloadLocation = downloadLocation;
    connect(socket, SIGNAL(readyRead()),
                this, SLOT(updateServerProgress()));
    connect(socket, &QTcpSocket::errorOccurred,
        this, &FileDownloader::displayError);
    downloadTotalBytes = 0;
    bytesReceived = 0;
    fileNameSize = 0;
    DEBUG
}

FileDownloader::~FileDownloader(){
    qDebug("~FileDownload!\n");
    delete downloadFile;
    socket->deleteLater();
}


void FileDownloader::updateServerProgress()
{
    DEBUG
    QDataStream in(socket);
    in.setVersion(QDataStream::Qt_4_0);

    // 如果接收到的數據小於16個字節，保存到來的文件頭結構
    if (bytesReceived <= sizeof(qint64)*2) {
        if ((socket->bytesAvailable() >= sizeof(qint64)*2) && (fileNameSize == 0)) {
            // 接收數據總大小信息和文件名大小信息
            in >> downloadTotalBytes >> fileNameSize;
            bytesReceived += sizeof(qint64) * 2;
        }
        if ((socket->bytesAvailable() >= fileNameSize) && (fileNameSize != 0)) {
            // 接收文件名，並建立文件
            in >> downloadFileName;
            cout << QStringLiteral("receive %1 ...\n").arg(downloadFileName);
            bytesReceived += fileNameSize;
            // cout << QStringLiteral("writing file '%1' ...\n").arg(downloadLocation + downloadFileName);
            cout.flush();
            downloadFile = new QFile(downloadLocation + downloadFileName);
            if (!downloadFile->open(QFile::WriteOnly)) {
                cerr << QStringLiteral("server: open file error!\n");
                cerr.flush();
                return;
            }
        } else {
            cerr << QStringLiteral("server: unkonwn error!\n");
            cerr.flush();
//            this->status = STATUS::LISTENING;
            return;
        }
    }
    // 如果接收的數據小於總數據，那麼寫入文件
    if (bytesReceived < downloadTotalBytes) {
        bytesReceived += socket->bytesAvailable();
        inBlock = socket->readAll();
        downloadFile->write(inBlock);
        inBlock.resize(0);
    }
    qDebug() << bytesReceived << "/" << downloadTotalBytes << '\n';
    // 接收數據完成時
    if (bytesReceived == downloadTotalBytes) {
        socket->close();
        downloadFile->close();
        cout << tr("receive file '%1' success!\n").arg(downloadFileName);
        cout << tr("save to %1！\n").arg(downloadLocation + downloadFileName);
        cout.flush();
        downloadTotalBytes = 0;
        bytesReceived = 0;
        fileNameSize = 0;
        emit finish(downloadLocation + downloadFileName);
    }
}



void FileDownloader::displayError()
{
    cout << "TCP Error: " << socket->errorString() << '\n';
    cout.flush();
    emit this->error();
}


void FileDownloader::setDownloadLocation(const QString& loc){
    this->downloadLocation = loc;
}
