#ifndef NODEVIEW_H
#define NODEVIEW_H

#include <QDialog>
#include "rem.h"
#include <QCloseEvent>

namespace Ui {
  class NodeView;
}

class Rem;
void *node_listener(void *_arg);

class NodeView : public QDialog
{
  Q_OBJECT

public:
  explicit NodeView(int pos, QWidget *parent = 0);
  ~NodeView();

  pthread_t listener_process;
  pthread_mutex_t listener_mutex;
  friend void *node_listener(void *);
  bool app_open = true;

  public slots:
  void updateView();


  private slots:
  void on_NodeView_destroyed();

  void on_NodeView_rejected();

private:
  Ui::NodeView *ui;
  Rem *rem;
  int pos;
};

#endif // NODEVIEW_H
