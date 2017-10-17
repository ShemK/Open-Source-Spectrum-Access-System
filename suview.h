#ifndef SUVIEW_H
#define SUVIEW_H

#include <QDialog>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <vector>
#include "rem.h"

using namespace QtCharts;

namespace Ui {
  class SuView;
}

class Rem;

void *su_listener(void *_arg);

class SuView : public QDialog
{
  Q_OBJECT

public:
  explicit SuView(int pos, QWidget *parent = 0);
  ~SuView();

  pthread_t listener_process;
  pthread_mutex_t listener_mutex;
  friend void *su_listener(void *);
  bool app_open = true;
  std::vector <double> bitrate;

  void createStatTable();
  std::string last_state = "";

public slots:
  void updateView();

private slots:

  void on_SuView_destroyed();

private:
  Ui::SuView *ui;
  Rem *rem;
  int pos;
};

#endif // SUVIEW_H
