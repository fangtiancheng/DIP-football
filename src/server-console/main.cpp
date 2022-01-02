#include <QCoreApplication>
#include "server.h"
int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    // listen to client
    Server server(nullptr);
    return a.exec();
}
