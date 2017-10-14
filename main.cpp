#include "rem.h"
#include <QApplication>

int main(int argc, char *argv[])
{
  QApplication a(argc, argv);
  Rem w;
  w.show();

  return a.exec();
}
