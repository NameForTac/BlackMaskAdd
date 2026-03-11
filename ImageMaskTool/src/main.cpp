#include "MainWindow.h"
#include <QApplication>
#include <QDir>
#include <QCoreApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // Ensure plugin path is set relative to executable
    QString appPath = QCoreApplication::applicationDirPath();
    app.addLibraryPath(appPath);
    app.addLibraryPath(appPath + "/plugins");

    MainWindow window;
    window.show();
    
    return app.exec();
}
