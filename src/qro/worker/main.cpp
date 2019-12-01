
#include <QCoreApplication>
#include "worker.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    SimpleWorker srcWorker;

    if (a.arguments().length() != 2) {
        qCritical() << "Invalid amount of arguments!";
        return 2;
    }

    QRemoteObjectHost srcNode(QUrl(a.arguments().at(1)));
    srcNode.enableRemoting(&srcWorker);

    return a.exec();
}
