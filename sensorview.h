#ifndef SENSORVIEW_H
#define SENSORVIEW_H

#include <QDialog>
#include "rem.h"

namespace Ui
{
class SensorView;
}

class Rem;

void *sensor_listener(void *_arg);

class SensorView : public QDialog
{
  Q_OBJECT

public:
  explicit SensorView(int pos, QWidget *parent = 0);
  ~SensorView();

  pthread_t listener_process;
  pthread_mutex_t listener_mutex;
  friend void *sensor_listener(void *);
  bool app_open = true;


public slots:
  void updateView();

private slots:
  void on_SensorView_destroyed();

  void on_buttonBox_rejected();

private:
  Ui::SensorView *ui;
  Rem *rem;
  int pos;
};

#endif // SENSORVIEW_H
