#include "server.h"
#define DEBUG qDebug() << "in file " << __FILE__ << ", line " << __LINE__;
Server::Server(QObject* father):
    QObject(father), server(new TcpServer(this)),
    connection(new Connection(this)),
    cmd(new QProcess(this)), cin(stdin, QIODevice::ReadOnly),
    cout(stdout, QIODevice::WriteOnly), cerr(stderr, QIODevice::WriteOnly)
{
    connect(cmd, &QProcess::readyReadStandardOutput,
            this, &Server::cmdReadOutput);
    connect(cmd, &QProcess::readyReadStandardError,
            this, &Server::cmdReadError);
    connect(connection, &Connection::newMessage,
            this, &Server::receiveMessage);
    connect(connection, &Connection::disconnected,
            this, &Server::closeConnection);
    // Listen to port 4399
    connect(server, &TcpServer::newConnection, this,
            &Server::receiveConnection);
    this->fileStatus = 0;
    // edit it
    downloadFileMap["JSON1"] = "./output/json/test_out_0.json";
    downloadFileMap["JSON2"] = "./output/json/test_out_1.json";
    downloadFileMap["OUTVIDEO"] = "./output/vis/demo.mp4";
    downloadFileMap["TEAM1VIDEO"] = "./output/test_out_0.mp4";
    downloadFileMap["TEAM2VIDEO"] = "./output/test_out_1.mp4";
    downloadFileMap["TRANSEDFRAME"] = "./trans_out/demo_field_net.jpg";
}

Server::~Server(){
    delete  this->cmd;
    delete  this->connection;
    delete  this->server;
}
void Server::closeConnection(){
    this->fileStatus = 0;
    cout << "Connection closed!\n";
    cout.flush();
}
void Server::appendLog(const QString& msg){
    cout << msg << '\n';
    cout.flush();
}
void Server::receiveConnection(qintptr socketDescriptor){
    qDebug() << "receive connection\n";
    if(this->connection->getState() != Connection::ReadyForUse){
        this->connection->setConnection(socketDescriptor);
        cout << tr("connect to client\n");
        cout.flush();
    }
    else{
        QTcpSocket* tmpSock = new QTcpSocket(this);
        tmpSock->setSocketDescriptor(socketDescriptor);
        qDebug() << "in line " <<  __LINE__;
        if((fileStatus & 0b11) == 0b10){
            DEBUG
            FileUploader* uploader = new FileUploader(tmpSock, this);
            uploader->setFileName(this->downloadFileName);
            connect(uploader, &FileUploader::finish, [uploader](){
                delete uploader;
            });
            uploader->startTransfer();
        }else if((fileStatus & 0b11) == 0b01){
            DEBUG
            // 1: next is upload from client to server
            FileDownloader* downloader = new FileDownloader(tmpSock, this, "");
            if((fileStatus & 0b1111) == 0b0101){
                DEBUG
                connect(downloader, &FileDownloader::finish, [this, downloader](QString fileName){
                    QFile::remove(fileName);
                    QDir inputFileDir("./inputfile");
                    if(!inputFileDir.exists()){
                        connection->sendMessage("directory \"./inputfile\" does not exist!");
                        if(inputFileDir.mkdir("./inputfile")){
                            connection->sendMessage("create dir \"./inputfile\"");
                        }
                        else{
                            connection->sendMessage("cannot create dir  \"./inputfile\", please fix it by command!");
                        }
                    }
                    bool ret = QFile::rename(fileName, "./inputfile/demo.mp4");
                    if(!ret){
                        connection->sendMessage(tr("cannot move %1 to ./inputfile/demo.mp4!").arg(fileName));
                    }else{
                        connection->sendMessage(tr("received %1 and move it to the right place!").arg(fileName));
                    }
                    downloader->deleteLater();
                });
            }
            else if((fileStatus & 0b1111) == 0b1001){
                DEBUG
                connect(downloader, &FileDownloader::finish, [this, downloader](){
                    QFile::remove("./trans_out/human_trans.pkl");
                    bool ret = QFile::rename("./human_trans.pkl", "./trans_out/human_trans.pkl");
                    if(!ret){
                        connection->sendMessage(tr("cannot move ./human_trans.pkl to ./trans_out/human_trans.pkl!"));
                    }
                    else{
                        connection->sendMessage(tr("received human_trans.pkl and move it to the right place!"));
                    }
                    downloader->deleteLater();
                });
            }
            else{
                DEBUG
                connect(downloader, &FileDownloader::finish, [downloader](){
                    downloader->deleteLater();
                });
            }
        }else{
            qDebug() << "ERROR";
            qDebug() << "fileStatus = " << fileStatus;
        }
        fileStatus = 0;
    }
}
void Server::receiveDownloadRequest(){

}

void Server::receiveUploadRequest(){

}

void Server::receiveMessage(const QString& from, const QString& message){
    // parse message
    cout << "receive " << message << '\n';
    cout.flush();
    QStringList msgs= message.split(' ');
    qDebug() << "msgs[0] = ---" << msgs[0] << "---\n";
    if(msgs[0] == "[OPERATION]"){
        DEBUG
        if(msgs.length()>1){
            DEBUG
            if(msgs[1] == "DOWNLOAD"){
                DEBUG
                if(msgs.length()>2){
                    DEBUG
                    auto findFileIter = downloadFileMap.find(msgs[2]);
                    if(findFileIter == downloadFileMap.end()){
                        connection->sendMessage(tr("invalid symbol %1").arg(msgs[2]));
                    }
                    else{
                        downloadFileName = *findFileIter;
                        qDebug() << "set fileName = " << downloadFileName << '\n';
                        // check fileName
                        QFileInfo downloadFileInfo(downloadFileName);
                        if(downloadFileInfo.isFile()){
                            fileStatus = 0b10;
                            connection->sendMessage("READY_DOWNLOAD");
                            qDebug() << "send READY_DOWNLOAD\n";
                        }
                        else{
                            qDebug() << "file doesn't exist!\n";
                            connection->sendMessage(tr("file %1 not found!").arg(downloadFileName));
                        }
                    }
                }
            }// end "DOWNLOAD"
            else if(msgs[1] == "UPLOAD"){
                DEBUG
                if(msgs.length()>2){
                    if(msgs[2] == "VIDEO"){
                        fileStatus = 0b0101;
                    }
                    else if(msgs[2] == "PKL"){
                        fileStatus = 0b1001;
                    }
                    else{
                        fileStatus = 0b0001;
                    }
                }
                connection->sendMessage("READY_UPLOAD");
            }
            else{
                // error
            }
        }
        else{
            // error
        }
    }
    else if(msgs[0] == "[INSTRUCTION]"){
        msgs.removeFirst();
        cout << message << '\n';
        cout.flush();
        if(msgs.length()!=0){
            cmd->setProgram(msgs[0]);
            msgs.removeFirst();
            if(msgs.length()!=0){
                cmd->setArguments(msgs);
            }
            cmd->start();
        }
    }
    else if(msgs[0] == "[MESSAGE]"){
        msgs.removeFirst();
        cout << message << '\n';
        cout.flush();

    }
    else {
        cerr << tr("[WARNING]: unknown message '%1' from %2\n").arg(message, from);
    }
}
void Server::downloadError(){
    static int errorTimes = 0;
    errorTimes++;
    if(errorTimes>3){
        errorTimes = 0;
        cerr << "download error over 3 times, stop!";
        cerr.flush();
    }
    else{
        // try again!
        // TODO:
    }

}
void Server::cmdReadOutput(){
    cout << "Ready to read cmd output\n";
    cout.flush();
    if(this->connection->getState() != Connection::ConnectionState::ReadyForUse){
        cout << "connection is not available now!\n";
        cout.flush();
        return;
    }
    QString output = QString::fromLocal8Bit(cmd->readAllStandardOutput());
    cout << output << '\n';
    cout.flush();
    if(output.back() == '\n'){
        output.chop(1);
    }
    connection->sendMessage(output);
}

void Server::cmdReadError(){
    if(this->connection->getState() != Connection::ConnectionState::ReadyForUse){
        cout << "connection is not available now!\n";
        cout.flush();
        return;
    }
    QString output = QString::fromLocal8Bit(cmd->readAllStandardError());
    connection->sendMessage(output);
}

void Server::downloadFinish(){
    // begin to run python!
    cout << "Download OK! Begin to process!\n";
    cout.flush();
    connection->sendMessage("Download OK! Begin to process!");
    this->exePython();
}
void Server::exePython(){
    QStringList args;
    args << "/media/ftc/H/DIP/test_demo";
    cmd->setProgram("source");
    cmd->setArguments(args);
    cmd->start();
}
void Server::processDataFinish(){

}
