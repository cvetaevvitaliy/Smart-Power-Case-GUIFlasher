#include <QCoreApplication>
#include <QApplication>
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QProgressBar>
#include "main_app.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    Main_App *main_app = new Main_App();

    main_app->Main_App_Show();

    return app.exec();
}
