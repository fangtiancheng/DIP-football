#include "config.h"
#include "ui_config.h"

#include <QtWidgets>

Config::Config(QWidget *parent, ConfigMessage* cfgmsg) :
    QDialog(parent),
    cfgMsg(cfgmsg),
    ui(new Ui::Config)
{
    ui->setupUi(this);
    printf("SET UP CONFIGURE\n");
    // connect buttons
    connect(ui->viewVideoButton, &QAbstractButton::clicked,
            this, &Config::clickViewVideoButton);
    connect(ui->buttonBox, &QDialogButtonBox::accepted,
            [this](){
        this->cfgMsg->videoPath = ui->videoPathEdit->text();
        this->cfgMsg->ip = ui->ipEdit->text();
        this->cfgMsg->port = ui->portEdit->text();
    });
}

Config::~Config()
{
    delete ui;
}

void Config::clickViewVideoButton(){
    QString videoName = QFileDialog::getOpenFileName(this, "Choose a football video", "/home");
    if(videoName.isEmpty()) return;
    ui->videoPathEdit->setText(videoName);
}

void Config::clickTryConnectButton(){
    ui->logBrowser->append("Sorry, this function has not been implemented yet");
}
