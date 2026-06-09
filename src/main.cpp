#include "gameengine.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    GameEngine eng;
    eng.show();

    short exit=a.exec();

    return exit;
}
