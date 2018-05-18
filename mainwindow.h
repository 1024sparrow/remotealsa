#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtWidgets>
#include <QTcpSocket>
#include <QAudioDeviceInfo>
#include <QScopedPointer>
#include <QAudioOutput>
#include <QAudioFormat>

class LogWindow;

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
  void createAudioDecodeFormatGroupBox();

  void playAudioData();
  void initializeAudio(QAudioDeviceInfo const& deviceInfo);

  QAudioFormat getAudioFormat() const;

private slots:
  void connectButtonReleased();
  void audioInMuteButtonReleased();
  void audioOutMuteButtonReleased();
  void audioInDeviceChanged(int index);
  void audioOutDeviceChanged(int index);

  // socket states
  void onSocketConnected();
  void onSocketReadyRead();
  void onSocketError(QAbstractSocket::SocketError socketError);

private:

  // connect
  QGroupBox*                    m_serverGroupBox;
  QPushButton*                  m_connectButton;
  QLineEdit*                    m_serverAddressLineEdit;
  QLineEdit*                    m_serverPortLineEdit;

  // audio output
  QGroupBox*                    m_audioInGroupBox;
  QPushButton*                  m_audioInMuteButton;
  QProgressBar*                 m_audioInProgressBar;
  QComboBox*                    m_audioInSelector;
  bool                          m_audioInMute;
  QAudioDeviceInfo              m_audioInDeviceInfo;

  // audio input
  QGroupBox*                    m_audioOutGroupBox;
  QPushButton*                  m_audioOutMuteButton;
  QProgressBar*                 m_audioOutProgressBar;
  QComboBox*                    m_audioOutSelector;
  bool                          m_audioOutMute;
  QScopedPointer<QAudioOutput>  m_audioOutput;
  QAudioFormat                  m_audioOutputFormat;
  QIODevice*                    m_audioOutDevice;
  QByteArray                    m_audioReadBuffer;

  // audio decode format settings
  QGroupBox*                    m_audioDecodeGroupBox;
  QLabel*                       m_audioDecodeSampleRateLabel;
  QLineEdit*                    m_audioDecodeSampleRateInput;
  QLabel*                       m_audioDecodeChannelsLabel;
  QLineEdit*                    m_audioDecodeChannelsInput;
  QLabel*                       m_audioDecodeSampleSizeLabel;
  QLineEdit*                    m_audioDecodeSampleSizeInput;
  QLabel*                       m_audioDecodeCodecLabel;
  QLineEdit*                    m_audioDecodeCodecInput;
  QLabel*                       m_audioDecodeByteOrderLabel;
  QComboBox*                    m_audioDecodeByteOrderSelector;
  QLabel*                       m_audioDecodeSampleTypeLabel;
  QComboBox*                    m_audioDecodeSampleTypeSelector;

  // feedback box
  LogWindow*                    m_logWindow;
  QDialogButtonBox*             m_dialogButtonBox;

  // client socket
  QTcpSocket*                   m_socket;
};

#endif // MAINWINDOW_H
