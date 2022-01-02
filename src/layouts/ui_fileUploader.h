/********************************************************************************
** Form generated from reading UI file 'fileUploader.ui'
**
** Created by: Qt User Interface Compiler version 6.2.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_FILEUPLOADER_H
#define UI_FILEUPLOADER_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QTextBrowser>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_Client
{
public:
    QGridLayout *gridLayout;
    QTextBrowser *textBrowser;
    QLineEdit *portLineEdit;
    QLineEdit *hostLineEdit;
    QPushButton *openButton;
    QProgressBar *clientProgressBar;
    QPushButton *sendButton;
    QPushButton *stopButton;
    QSpacerItem *horizontalSpacer;

    void setupUi(QWidget *Client)
    {
        if (Client->objectName().isEmpty())
            Client->setObjectName(QString::fromUtf8("Client"));
        Client->resize(500, 320);
        gridLayout = new QGridLayout(Client);
        gridLayout->setSpacing(6);
        gridLayout->setContentsMargins(11, 11, 11, 11);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        textBrowser = new QTextBrowser(Client);
        textBrowser->setObjectName(QString::fromUtf8("textBrowser"));

        gridLayout->addWidget(textBrowser, 3, 0, 1, 5);

        portLineEdit = new QLineEdit(Client);
        portLineEdit->setObjectName(QString::fromUtf8("portLineEdit"));

        gridLayout->addWidget(portLineEdit, 1, 4, 1, 1);

        hostLineEdit = new QLineEdit(Client);
        hostLineEdit->setObjectName(QString::fromUtf8("hostLineEdit"));

        gridLayout->addWidget(hostLineEdit, 1, 0, 1, 4);

        openButton = new QPushButton(Client);
        openButton->setObjectName(QString::fromUtf8("openButton"));

        gridLayout->addWidget(openButton, 4, 0, 1, 1);

        clientProgressBar = new QProgressBar(Client);
        clientProgressBar->setObjectName(QString::fromUtf8("clientProgressBar"));
        clientProgressBar->setValue(0);

        gridLayout->addWidget(clientProgressBar, 2, 0, 1, 5);

        sendButton = new QPushButton(Client);
        sendButton->setObjectName(QString::fromUtf8("sendButton"));

        gridLayout->addWidget(sendButton, 4, 4, 1, 1);

        stopButton = new QPushButton(Client);
        stopButton->setObjectName(QString::fromUtf8("stopButton"));

        gridLayout->addWidget(stopButton, 4, 3, 1, 1);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout->addItem(horizontalSpacer, 4, 1, 1, 1);


        retranslateUi(Client);

        QMetaObject::connectSlotsByName(Client);
    } // setupUi

    void retranslateUi(QWidget *Client)
    {
        Client->setWindowTitle(QCoreApplication::translate("Client", "Client", nullptr));
        portLineEdit->setText(QCoreApplication::translate("Client", "6666", nullptr));
        hostLineEdit->setText(QCoreApplication::translate("Client", "127.0.0.1", nullptr));
        openButton->setText(QCoreApplication::translate("Client", "open", nullptr));
        sendButton->setText(QCoreApplication::translate("Client", "send", nullptr));
        stopButton->setText(QCoreApplication::translate("Client", "stop", nullptr));
    } // retranslateUi

};

namespace Ui {
    class Client: public Ui_Client {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_FILEUPLOADER_H
