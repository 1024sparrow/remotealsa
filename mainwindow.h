#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtWidgets>
#include <QTcpSocket>
#include <QAudioDeviceInfo>
#include <QScopedPointer>
#include <QAudioOutput>
#include <QAudioFormat>

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

  void playAudioData();
  void initializeAudio(QAudioDeviceInfo const& deviceInfo);

private slots:
  void connectButtonReleased();
  void audioInMuteButtonReleased();
  void audioOutMuteButtonReleased();
  void audioInDeviceChanged(int index);
  void audioOutDeviceChanged(int index);

  // socket states
  void onSocketConnected();
  void onSocketReadyRead();

private:
  QGroupBox*                    m_serverGroupBox;
  QPushButton*                  m_connectButton;
  QLineEdit*                    m_serverAddressLineEdit;
  QLineEdit*                    m_serverPortLineEdit;

  QGroupBox*                    m_audioInGroupBox;
  QPushButton*                  m_audioInMuteButton;
  QProgressBar*                 m_audioInProgressBar;
  QComboBox*                    m_audioInSelector;
  bool                          m_audioInMute;
  QAudioDeviceInfo              m_audioInDeviceInfo;

  QGroupBox*                    m_audioOutGroupBox;
  QPushButton*                  m_audioOutMuteButton;
  QProgressBar*                 m_audioOutProgressBar;
  QComboBox*                    m_audioOutSelector;
  bool                          m_audioOutMute;
  QScopedPointer<QAudioOutput>  m_audioOutput;
  QAudioFormat                  m_audioOutputFormat;
  QIODevice*                    m_audioOutDevice;
  QByteArray                    m_audioReadBuffer;

  QTextEdit*                    m_feedbackText;
  QDialogButtonBox*             m_dialogButtonBox;

  QTcpSocket*                   m_socket;
};

#endif // MAINWINDOW_H
