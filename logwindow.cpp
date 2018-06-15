#include "logwindow.h"

#include <QDateTime>
#include <QScrollBar>

LogWindow::LogWindow()
{
  setReadOnly(true);
}

void
LogWindow::appendMessage(QString const& message)
{
  QDateTime const now = QDateTime::currentDateTime();
  QString text = QString("%1 : %2").arg(now.toString()).arg(message);

  appendPlainText(text);
  verticalScrollBar()->setValue(verticalScrollBar()->maximum());
}
