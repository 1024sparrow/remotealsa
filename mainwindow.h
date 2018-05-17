#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtWidgets>
#include <QTcpSocket>

class MainWindow : public QDialog
{
  Q_OBJECT

public:
  explicit MainWindow();
  ~MainWindow();

private:
  void createServerGroupBox();
  void createAudioInGroupBox();
  void createAudioOutGroupBox();

private slots:
  void connectButtonReleased();
  void audioInMuteButtonReleased();
  void audioOutMuteButtonReleased();


private:
  QGroupBox*   m_serverGroupBox;
  QPushButton* m_connectButton;
  QLineEdit*   m_serverLineEdit;

  QGroupBox*   m_audioInGroupBox;
  QPushButton* m_audioInMuteButton;
  QProgressBar*  m_audioInProgressBar;
  QComboBox*   m_audioInSelector;

  QGroupBox*   m_audioOutGroupBox;
  QPushButton* m_audioOutMuteButton;
  QProgressBar* m_audioOutProgressBar;

  QTextEdit* m_feedbackText;
  QDialogButtonBox* m_dialogButtonBox;

  QTcpSocket* m_socket;
};

#endif // MAINWINDOW_H
