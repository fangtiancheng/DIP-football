#include "mainWidget.h"

mainWidget::mainWidget(){
    this->config = new ConfigMessage();
    this->connection = new Connection(this);
    this->fileUploader = new FileUploader(nullptr);
    this->dialogWidget = new Dialog(config, fileUploader, connection);
    this->playerWidget = new Player(nullptr, config, connection, fileUploader);
    connect(dialogWidget, &Dialog::accepted,[this](){
        this->playerWidget->updateSource();
        this->playerWidget->show();
    });
}
mainWidget::~mainWidget(){
    dialogWidget->deleteLater();
    playerWidget->deleteLater();
}
void mainWidget::show(){
    this->dialogWidget->show();
}
