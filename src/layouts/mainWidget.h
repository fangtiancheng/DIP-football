#ifndef MAINWIDGET_H
#define MAINWIDGET_H
#include <QDialog>
#include "common.h"
#include "fileUploader.h"
#include "connection.h"
#include "dialog.h"
#include "player/player.h"

class mainWidget: public QObject
{
    Q_OBJECT
public:
    mainWidget();
    ~mainWidget();
    void show();
private:
    // Widget Module
    Dialog* dialogWidget;
    Player* playerWidget;
    // Connection Module
    FileUploader* fileUploader;
    Connection* connection;
    // Config Message
    ConfigMessage* config;
};

#endif // MAINWIDGET_H
