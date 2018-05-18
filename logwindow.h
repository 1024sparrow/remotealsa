#ifndef LOGWINDOW_H
#define LOGWINDOW_H

#include <QWidget>
#include <QPlainTextEdit>

class LogWindow : public QPlainTextEdit
{
  Q_OBJECT

public:
  LogWindow();
  void appendMessage(QString const& message);
};

#endif // LOGWINDOW_H
