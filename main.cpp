#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QStringList paths = QCoreApplication::libraryPaths();
    paths.append("platforms");
    QCoreApplication::setLibraryPaths(paths);

    QApplication a(argc, argv);
    MainWindow *w = new MainWindow();
    w->setWindowTitle("Impairor_1.0.3");
    w->setAttribute(Qt::WA_DeleteOnClose);
    w->show();
    return a.exec();
}
