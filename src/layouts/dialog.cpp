/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtWidgets>
#include "dialog.h"
#include "config.h"
#include "common.h"
#include <QAbstractSocket>
// ================================================================================

// =======================================================================================
void Dialog::clickNewProject() {
    QString dirName = QFileDialog::getExistingDirectory(this, "Choose a directory", "/home/ftc/football/data");
    if(dirName.isEmpty()) return;
    this->config->dirName = dirName;
    Config* cfgBox = new Config(this, this->config);
    cfgBox->show();
    int ret = cfgBox->exec();
    cfgBox->deleteLater();
    if(ret){
        // choose OK
        // QMessageBox::information(this, "info", this->config->toJson());
        // save to file
        QFile json(this->config->dirName+"/config.json");
        if(!json.open(QFile::WriteOnly| QFile::Text)){
            QMessageBox::warning(this, "Error", tr("Cannot write json to %s/config.json").arg(this->config->dirName));
            return;
        }
        else{
            QTextStream out(&json);
            out << this->config->toJson();
            smallEditor->setPlainText(this->config->toJson());
        }
        videoPathEdit->setText(config->videoPath);
        ipEdit->setText(config->ip);
        portEdit->setText(config->port);
        projectPathEdit->setText(dirName);
    }
    else{
        // choose cancel
    }
    // QMessageBox::information(this, "Test", QString::number(ret));
}
void Dialog::clickOpenExist(){
    printf("Click Open Exist\n");
    QString dirName = QFileDialog::getExistingDirectory(this, "Choose a directory", "/home/ftc/football/data");
    if(dirName.isEmpty()) return;
    this->config->dirName = dirName;
    this->projectPathEdit->setText(dirName);
    bool succ = this->config->load();
    if(!succ){
        // TODO: sth
        QMessageBox::warning(this, "Load Error", tr("Load Error from %s/config.json").arg(dirName));
    }
    else{
        // load success
//        QMessageBox::information(this, "Load Success", QString::fromStdString(this->config.toJson()));
        smallEditor->setPlainText(this->config->toJson());
        generateEdit();
    }
}
void Dialog::clickHelp(){
    printf("Click Help\n");
    QMessageBox::information(this, "Help Message", "This is a test.");
}

void Dialog::printSocketState(QAbstractSocket::SocketState stt, const char* file, int line){
    const char* state[] = {
        "UnconnectedState",
        "HostLookupState",
        "ConnectingState",
        "ConnectedState",
        "BoundState",
        "ListeningState",
        "ClosingState"
    };
    QMessageBox::information(this, "STATE", tr("%1 in file %2, line %3").arg(state[stt], QString::fromStdString(file), QString::number(line)));
}


void Dialog::appendLog(QString log){
    qDebug("receive appendLog signal!\n");
    bigEditor->append(log);
}

void Dialog::receiveMessage(const QString &from, const QString &message){
    if (from.isEmpty() || message.isEmpty())
            return;

    QStringList msgs = message.split(" ");
    if(msgs[0] == "READY_UPLOAD"){
        qDebug() << "READY_UPLOAD\n";
        dataClient->connectToHost(config->ip, config->port.toInt());
        dataClient->setFileName(uploadFileName);
        dataClient->startTransfer();
        bigEditor->append(tr("start upload %1").arg(uploadFileName));
        uploadFileName = "";
    }
    else if(msgs[0] == "READY_DOWNLOAD"){
        QTcpSocket* tmpSock = new QTcpSocket(this);
        tmpSock->connectToHost(config->ip, config->port.toInt());
        FileDownloader* downloader = new FileDownloader(tmpSock, this, projectPathEdit->text() + "/");
        connect(downloader, &FileDownloader::finish, [downloader, this](QString downloadFilePath){
            auto iter = downloadFileToConfig.find(downloadFileType);
            if(iter != downloadFileToConfig.end()){
                **iter = downloadFilePath;
                this->smallEditor->setText(config->toJson());
            }
            downloader->deleteLater();
            downloadFileType = "";
        });
        connect(downloader, &FileDownloader::log, this, &Dialog::appendLog);
    }
    else{
        // unknow
        bigEditor->append(message);
    }

}
void Dialog::clickSendVideo(){
    QString targetFile = chooseUploadBox->currentData().toString();
    bigEditor->append(tr("click Upload %1").arg(targetFile));
    if(targetFile == "VIDEO"){
        uploadFileName = config->videoPath;
    }
    else if(targetFile == "PKL"){
        uploadFileName = "./GUI/out/human_trans.pkl";
    }
    else{
        // error

    }
    // send
    connection->sendMessage(tr("[OPERATION] UPLOAD %1").arg(targetFile));
}
void Dialog::clickDownloadResult(){
    QString targetFile = chooseDownloadBox->currentData().toString();
    bigEditor->append(tr("You try to download %1").arg(targetFile));
    connection->sendMessage(tr("[OPERATION] DOWNLOAD %1").arg(targetFile));
    downloadFileType = targetFile;
}
void Dialog::clickAnalyseVideoButton(){
    bigEditor->append("analyse video begin!");
    QString targetFile = chooseAnalyseBox->currentData().toString();
    connection->sendMessage(tr("[INSTRUCTION] bash %1").arg(targetFile));
}

void Dialog::generateConfig(){
    if(projectPathEdit->text().simplified().isEmpty()) return;
    if(config->ip != ipEdit->text() || config->port != portEdit->text()){
        if(connection->getState()==Connection::ConnectionState::ReadyForUse){
            this->connection->close();
        }
    }
    this->config->videoPath = this->videoPathEdit->text();
    this->config->ip = this->ipEdit->text();
    this->config->port = this->portEdit->text();
    smallEditor->setPlainText(this->config->toJson());
    generateEdit();
    // save to json
    this->config->save();
}
void Dialog::clickConnectServer(){
    const int timeOut = 50000;
    if(connection->state() == QAbstractSocket::UnconnectedState){
            connection->connectToHost(config->ip, config->port.toInt());
        // note: there is no break!
            if(!connection->waitForConnected(timeOut)){
                QMessageBox::warning(this, tr("Warning"), tr("Connect Time Out"));
                bigEditor->append(tr("try connect to server(ip = %1, port = %2). Time out!")
                          .arg(config->ip, config->port));
            }
     }
     else{

        bool reconnect = QMessageBox::question(this, "Reconnect",
tr("The client is already connect to server(ip = %1, port = %2).\nDo you want to close it first?")
                                          .arg(config->ip, config->port),
                QMessageBox::Yes|QMessageBox::No) == QMessageBox::Yes;
        if(reconnect){
            connection->close();
        }
    }
}
void Dialog::clickExecuteCommandButton(){
    qDebug()<<"clickExecuteCommandButton()\n";
    QString command = commandEdit->text();
    commandEdit->clear();
    connection->sendMessage(tr("[INSTRUCTION] %1").arg(command));
}
void Dialog::connectedToServer(){
    // log
    bigEditor->append(tr("connect to server ip = %1, port = %2").arg(config->ip, config->port));
    // set button status
    downloadResultButton->setEnabled(true);
    chooseDownloadBox->setEnabled(true);
    chooseAnalyseBox->setEnabled(true);
    chooseUploadBox->setEnabled(true);
    sendVideoButton->setEnabled(true);
    executeCommandButton->setEnabled(true);
    commandEdit->setEnabled(true);
    analyseButton->setEnabled(true);
    downloadResultButton->setEnabled(true);
}
void Dialog::disconnectToServer(){
    // log
    bigEditor->append(tr("disconnected"));
    // set button status
    downloadResultButton->setEnabled(false);
    chooseDownloadBox->setEnabled(false);
    chooseAnalyseBox->setEnabled(false);
    chooseUploadBox->setEnabled(false);
    sendVideoButton->setEnabled(false);
    executeCommandButton->setEnabled(false);
    commandEdit->setEnabled(false);
    analyseButton->setEnabled(false);
    downloadResultButton->setEnabled(false);
//     refresh connection
    connection->deleteLater();
    connection = new Connection(this);
    connect(connection, &Connection::readyForUse,
            this, &Dialog::connectedToServer);
    connect(connection, &Connection::disconnected,
            this, &Dialog::disconnectToServer);
    connect(connection, &Connection::errorOccurred,
            this, &Dialog::disconnectToServer);
    connect(connection, &Connection::newMessage,
            this, &Dialog::receiveMessage);
}
void Dialog::generateEdit(){
    this->videoPathEdit->setText(this->config->videoPath);
    this->ipEdit->setText(this->config->ip);
    this->portEdit->setText(this->config->port);
}

void Dialog::aboutAuthor(){
    QMessageBox::information(this, tr("About Author"),
       tr("Lu Xudong: run model\nFang Tiancheng: develop GUI"));
}

void Dialog::aboutProject(){
    QMessageBox::information(this, tr("About Project"),
                             tr("Something about football"));
}

// =======================================================================================
Dialog::~Dialog(){
    disconnect(config, &ConfigMessage::error, this, nullptr);
}
//! [0]
Dialog::Dialog(ConfigMessage* cfgmsg, FileUploader* clt, Connection* cnt):
    config(cfgmsg), connection(cnt), dataClient(clt)
{
    connect(config, &ConfigMessage::error,
        [this](const QString& msg){
            QMessageBox::warning(this,"error from config", msg);
        }
    );
    createMenu();
    createHorizontalGroupBox();
    createGridGroupBox();
    createFormGroupBox();
    createCommandBox();
    downloadFileToConfig["JSON1"] = &(config->json1Path);
    downloadFileToConfig["JSON2"] = &(config->json2Path);
    downloadFileToConfig["TEAM1VIDEO"] = &(config->result1Path);
    downloadFileToConfig["TEAM2VIDEO"] = &(config->result2Path);
//    this->client = new Client(this, this->socket);
//! [0]

//! [1]
    bigEditor = new QTextBrowser;
    bigEditor->setPlainText(tr("This widget takes up all the remaining space "
                               "in the top-level layout."));

    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok
                                     | QDialogButtonBox::Cancel);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(newProjectButton, &QAbstractButton::clicked,
            this, &Dialog::clickNewProject);
    connect(openExistButton, &QAbstractButton::clicked,
            this, &Dialog::clickOpenExist);
    connect(helpButton, &QAbstractButton::clicked,
            this, &Dialog::clickHelp);
    connect(reconfigButton, &QAbstractButton::clicked,
            this, &Dialog::generateConfig);
    connect(sendVideoButton, &QAbstractButton::clicked,
            this, &Dialog::clickSendVideo);
    connect(downloadResultButton, &QAbstractButton::clicked,
            this, &Dialog::clickDownloadResult);
    connect(connectServerButton, &QAbstractButton::clicked,
            this, &Dialog::clickConnectServer);
    connect(executeCommandButton, &QAbstractButton::clicked,
            this, &Dialog::clickExecuteCommandButton);
    connect(analyseButton, &QAbstractButton::clicked,
            this, &Dialog::clickAnalyseVideoButton);
    connect(commandEdit, &QLineEdit::returnPressed,
            this, &Dialog::clickExecuteCommandButton);
    // connect tcp
    connect(connection, &Connection::readyForUse,
            this, &Dialog::connectedToServer);
    connect(connection, &Connection::disconnected,
            this, &Dialog::disconnectToServer);
    connect(connection, &Connection::errorOccurred,
            this, &Dialog::disconnectToServer);
    connect(connection, &Connection::newMessage,
            this, &Dialog::receiveMessage);

//! [1]

//! [2]
    QVBoxLayout *mainLayout = new QVBoxLayout;
//! [2] //! [3]
    mainLayout->setMenuBar(menuBar);
//! [3] //! [4]
    mainLayout->addWidget(horizontalGroupBox);
    mainLayout->addWidget(gridGroupBox);
    mainLayout->addWidget(connectServerGroupBox);
    mainLayout->addWidget(sendCommandBox);
    mainLayout->addWidget(bigEditor);
    mainLayout->addWidget(buttonBox);
//! [4] //! [5]
    setLayout(mainLayout);
//    this->dataClient->setBrowser(bigEditor);
    connect(dataClient, &FileUploader::log,
        [this](const QString& message){
            this->bigEditor->append(message);
    });
    setWindowTitle(tr("Football Analyse Client"));
}
//! [5]

//! [6]
void Dialog::closeEvent(QCloseEvent * e){
    if( QMessageBox::question(this,
                                 tr("Quit"),
                                 tr("Are you sure to quit this application?"),
                                  QMessageBox::Yes, QMessageBox::No )
                       == QMessageBox::Yes){
            e->accept();//不会将事件传递给组件的父组件
            qApp->quit();
        }
        else
          e->ignore();
}
void Dialog::createMenu()
{
    menuBar = new QMenuBar;

    fileMenu = new QMenu(tr("&About"), this);
    aboutAuthorAction = fileMenu->addAction(tr("&Author"));
    aboutProjectAction = fileMenu->addAction(tr("&Project"));
    menuBar->addMenu(fileMenu);
    connect(aboutAuthorAction, &QAction::triggered, this, &Dialog::aboutAuthor);
    connect(aboutProjectAction, &QAction::triggered, this, &Dialog::aboutProject);
//    connect(exitAction, &QAction::triggered, this, &QDialog::accept);
}
//! [6]

//! [7]
void Dialog::createHorizontalGroupBox()
{
    horizontalGroupBox = new QGroupBox(tr("Configure Project"));
    QHBoxLayout *layout = new QHBoxLayout;
    newProjectButton = new QPushButton(tr("New Project"));
    openExistButton = new QPushButton(tr("Open Exist"));
    helpButton = new QPushButton(tr("Help"));
    layout->addWidget(newProjectButton);
    layout->addWidget(openExistButton);
    layout->addWidget(helpButton);

    horizontalGroupBox->setLayout(layout);
}
//! [7]

//! [8]
void Dialog::createGridGroupBox()
{
    gridGroupBox = new QGroupBox(tr("configuration"));
//! [8]
    QGridLayout *layout = new QGridLayout;

//! [9]
    projectPathLabel = new QLabel(tr("projectPath"));
    videoPathLabel = new QLabel(tr("videoPath"));
    ipLabel = new QLabel(tr("ip"));
    portLabel = new QLabel(tr("port"));
    layout->addWidget(projectPathLabel, 0, 0);
    layout->addWidget(videoPathLabel, 1, 0);
    layout->addWidget(ipLabel, 2, 0);
    layout->addWidget(portLabel, 3, 0);
    projectPathEdit = new QLineEdit(this);
    projectPathEdit->setReadOnly(true);
    videoPathEdit = new QLineEdit(this);
    portEdit = new QLineEdit(this);
    ipEdit = new QLineEdit(this);
    {// set validator
        QString ipRangeStr = "(?:[0-1]?[0-9]?[0-9]|2[0-4][0-9]|25[0-5])";//限制最大输入255
        QRegularExpression ipRegex ("^" + ipRangeStr
                              + "\\." + ipRangeStr
                              + "\\." + ipRangeStr
                              + "\\." + ipRangeStr + "$");
        auto ipValidator = new QRegularExpressionValidator(ipRegex,this);//检查其有效性
        ipEdit->setValidator(ipValidator);
        portEdit->setValidator(new QIntValidator(0, 65536,this));
    }

    layout->addWidget(projectPathEdit, 0, 1);
    layout->addWidget(videoPathEdit, 1, 1);
    layout->addWidget(ipEdit, 2, 1);
    layout->addWidget(portEdit, 3, 1);

    this->reconfigButton = new QPushButton(tr("generate config"));
    this->connectServerButton = new QPushButton(tr("Connect Server"));
    layout->addWidget(this->reconfigButton, 4, 0, 1, 2);
    layout->addWidget(this->connectServerButton, 5, 0, 1, 2);

//! [9] //! [10]
    smallEditor = new QTextBrowser;
    smallEditor->setPlainText(this->config->toJson());
    layout->addWidget(smallEditor, 0, 2, 6, 1);
//! [10]

//! [11]
    layout->setColumnStretch(1, 10);
    layout->setColumnStretch(2, 20);
    gridGroupBox->setLayout(layout);
}
//! [11]

//! [12]
void Dialog::createCommandBox(){
    sendCommandBox = new QGroupBox(this);
    QGridLayout *layout = new QGridLayout;
    chooseAnalyseBox = new QComboBox(this);
    chooseAnalyseBox->addItem(tr("step1: demo_video_trans.sh"), tr("demo_video_trans.sh"));
    chooseAnalyseBox->addItem(tr("step2 plan A: demo_video_human.sh"), tr("demo_video_human.sh"));
    chooseAnalyseBox->addItem(tr("step2 plan B: demo_video_net.sh"), tr("demo_video_net.sh"));
    chooseAnalyseBox->setEditable(false);
    chooseAnalyseBox->setEnabled(false);
    commandEdit = new QLineEdit(this);
    commandEdit->setEnabled(false);
    layout->addWidget(commandEdit, 0, 0);
    executeCommandButton = new QPushButton(this);
    executeCommandButton->setText(tr("Execute"));
    executeCommandButton->setEnabled(false);
    layout->addWidget(executeCommandButton, 0, 1);
    analyseButton = new QPushButton(this);
    analyseButton->setText(tr("Analyse Video"));
    analyseButton->setEnabled(false);
    layout->addWidget(chooseAnalyseBox, 1, 0);
    layout->addWidget(analyseButton, 1, 1);
    sendCommandBox->setLayout(layout);

}
void Dialog::createFormGroupBox()
{
        connectServerGroupBox = new QGroupBox(tr("Connect Server"));
        QGridLayout *layout = new QGridLayout;
        sendVideoButton = new QPushButton(tr("Upload"));
        downloadResultButton = new QPushButton(tr("Download"));

        chooseDownloadBox = new QComboBox(this);
        chooseDownloadBox->addItem(tr("json of team 1"), tr("JSON1"));
        chooseDownloadBox->addItem(tr("json of team 2"), tr("JSON2"));
        chooseDownloadBox->addItem(tr("analysed video"), tr("OUTVIDEO"));
        chooseDownloadBox->addItem(tr("video of team 1"), tr("TEAM1VIDEO"));
        chooseDownloadBox->addItem(tr("video of team 2"), tr("TEAM2VIDEO"));
        chooseDownloadBox->addItem(tr("transformed frame"), tr("TRANSEDFRAME"));

        chooseUploadBox = new QComboBox(this);
        chooseUploadBox->addItem(tr("video"), tr("VIDEO"));
        chooseUploadBox->addItem(tr("pkl"), tr("PKL"));

        layout->addWidget(chooseDownloadBox, 0, 0);
        layout->addWidget(downloadResultButton, 0, 1);
        layout->addWidget(chooseUploadBox, 1, 0);
        layout->addWidget(sendVideoButton, 1, 1);

        sendVideoButton->setEnabled(false);
        downloadResultButton->setEnabled(false);
        chooseDownloadBox->setEnabled(false);
        chooseDownloadBox->setEditable(false);
        chooseUploadBox->setEnabled(false);
        chooseUploadBox->setEditable(false);
        connectServerGroupBox->setLayout(layout);
}
//! [12]
