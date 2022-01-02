#ifndef CONFIG_H
#define CONFIG_H
#include "common.h"
#include <QDialog>

namespace Ui {
class Config;
}

class Config : public QDialog
{
    Q_OBJECT

public:
    explicit Config(QWidget *parent, ConfigMessage*);
    ~Config();
private slots:
    void clickViewVideoButton();
    void clickTryConnectButton();
private:
    ConfigMessage* cfgMsg;
    Ui::Config *ui;
};

#endif // CONFIG_H
