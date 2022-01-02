/********************************************************************************
** Form generated from reading UI file 'config.ui'
**
** Created by: Qt User Interface Compiler version 6.2.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_CONFIG_H
#define UI_CONFIG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTextBrowser>

QT_BEGIN_NAMESPACE

class Ui_Config
{
public:
    QDialogButtonBox *buttonBox;
    QPushButton *viewVideoButton;
    QLineEdit *videoPathEdit;
    QLabel *label;
    QLineEdit *ipEdit;
    QLineEdit *portEdit;
    QLabel *label_2;
    QLabel *label_3;
    QTextBrowser *logBrowser;
    QPushButton *tryConnectButton;

    void setupUi(QDialog *Config)
    {
        if (Config->objectName().isEmpty())
            Config->setObjectName(QString::fromUtf8("Config"));
        Config->resize(511, 297);
        buttonBox = new QDialogButtonBox(Config);
        buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
        buttonBox->setGeometry(QRect(130, 250, 341, 32));
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);
        viewVideoButton = new QPushButton(Config);
        viewVideoButton->setObjectName(QString::fromUtf8("viewVideoButton"));
        viewVideoButton->setGeometry(QRect(40, 30, 81, 31));
        videoPathEdit = new QLineEdit(Config);
        videoPathEdit->setObjectName(QString::fromUtf8("videoPathEdit"));
        videoPathEdit->setGeometry(QRect(130, 30, 341, 31));
        label = new QLabel(Config);
        label->setObjectName(QString::fromUtf8("label"));
        label->setGeometry(QRect(30, 10, 171, 17));
        ipEdit = new QLineEdit(Config);
        ipEdit->setObjectName(QString::fromUtf8("ipEdit"));
        ipEdit->setGeometry(QRect(40, 80, 241, 25));
        portEdit = new QLineEdit(Config);
        portEdit->setObjectName(QString::fromUtf8("portEdit"));
        portEdit->setGeometry(QRect(292, 80, 181, 25));
        label_2 = new QLabel(Config);
        label_2->setObjectName(QString::fromUtf8("label_2"));
        label_2->setGeometry(QRect(50, 60, 16, 17));
        label_3 = new QLabel(Config);
        label_3->setObjectName(QString::fromUtf8("label_3"));
        label_3->setGeometry(QRect(290, 60, 31, 17));
        logBrowser = new QTextBrowser(Config);
        logBrowser->setObjectName(QString::fromUtf8("logBrowser"));
        logBrowser->setGeometry(QRect(40, 120, 311, 111));
        tryConnectButton = new QPushButton(Config);
        tryConnectButton->setObjectName(QString::fromUtf8("tryConnectButton"));
        tryConnectButton->setGeometry(QRect(370, 120, 101, 111));

        retranslateUi(Config);
        QObject::connect(buttonBox, &QDialogButtonBox::accepted, Config, qOverload<>(&QDialog::accept));
        QObject::connect(buttonBox, &QDialogButtonBox::rejected, Config, qOverload<>(&QDialog::reject));

        QMetaObject::connectSlotsByName(Config);
    } // setupUi

    void retranslateUi(QDialog *Config)
    {
        Config->setWindowTitle(QCoreApplication::translate("Config", "Dialog", nullptr));
        viewVideoButton->setText(QCoreApplication::translate("Config", "View", nullptr));
        label->setText(QCoreApplication::translate("Config", "Select a video", nullptr));
        label_2->setText(QCoreApplication::translate("Config", "ip", nullptr));
        label_3->setText(QCoreApplication::translate("Config", "port", nullptr));
        tryConnectButton->setText(QCoreApplication::translate("Config", "try \n"
" connect", nullptr));
    } // retranslateUi

};

namespace Ui {
    class Config: public Ui_Config {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_CONFIG_H
