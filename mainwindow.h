#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtWidgets>

#include <QAudioDecoder>
#include <QAudioDeviceInfo>
#include <QAudioFormat>
#include <QAudioInput>
#include <QAudioOutput>
#include <QBuffer>
#include <QScopedPointer>
#include <QTcpSocket>
#include <QUdpSocket>


class LogWindow;

class AudioSource : public QIODevice
{
  Q_OBJECT

public:
  AudioSource();

  qint64 readData(char* data, qint64 maxLen) override;
  qint64 writeData(char const* data, qint64 len) override;
  qint64 bytesAvailable() const override;

  void append(QByteArray const& buff)
    { m_data.append(buff); }

 private:
  QByteArray m_data;
};

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
  void createAudioEncodeFormatGroupBox();

  void playAudioData();

  void initializeAudioOutputDevice(QAudioDeviceInfo const& deviceInfo);
  void initializeAudioInputDevice(QAudioDeviceInfo const& deviceInfo);

  QAudioFormat getAudioOutputFormat() const;
  QAudioFormat getAudioInputFormat() const;

private slots:
  void connectButtonReleased();
  void reconnectToHost();
  void audioInMuteButtonReleased();
  void audioOutMuteButtonReleased();
  void audioInDeviceChanged(int index);
  void audioOutDeviceChanged(int index);
  void onEncodeVolumeValueChanged(int value);
  void onIncomingSoundData();

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

  // audio in
  QGroupBox*                    m_audioInGroupBox;
  QPushButton*                  m_audioInMuteButton;
  QProgressBar*                 m_audioInProgressBar;
  QComboBox*                    m_audioInSelector;
  bool                          m_audioInMute;
  bool                          m_audioFromFile;
  QAudioDeviceInfo              m_audioInDeviceInfo;
  QScopedPointer<QAudioInput>   m_audioInput;
  QIODevice*                    m_audioInputDevice;
  QByteArray                    m_audioWriteBuffer;
  QRadioButton*                 m_audioInFromDeviceRadioButton;
  QRadioButton*                 m_audioInFromFileRadioButton;
  QPushButton*                  m_audioInOpenFilePushButton;
  QAudioDecoder                 m_audioInputDecoder;

  // audio decode format settings
  QGroupBox*                    m_audioEncodeGroupBox;
  QLabel*                       m_audioEncodeSampleRateLabel;
  QLineEdit*                    m_audioEncodeSampleRateInput;
  QLabel*                       m_audioEncodeChannelsLabel;
  QLineEdit*                    m_audioEncodeChannelsInput;
  QLabel*                       m_audioEncodeSampleSizeLabel;
  QLineEdit*                    m_audioEncodeSampleSizeInput;
  QLabel*                       m_audioEncodeCodecLabel;
  QLineEdit*                    m_audioEncodeCodecInput;
  QLabel*                       m_audioEncodeByteOrderLabel;
  QComboBox*                    m_audioEncodeByteOrderSelector;
  QLabel*                       m_audioEncodeSampleTypeLabel;
  QComboBox*                    m_audioEncodeSampleTypeSelector;
  QSlider*                      m_audioEncodeVolumeSlider;


  // audio out
  QGroupBox*                    m_audioOutGroupBox;
  QPushButton*                  m_audioOutMuteButton;
  QProgressBar*                 m_audioOutProgressBar;
  QComboBox*                    m_audioOutSelector;
  bool                          m_audioOutMute;
  QScopedPointer<QAudioOutput>  m_audioOutput;
  QAudioFormat                  m_audioOutputFormat;
  QIODevice*                    m_audioOutDevice;
  QByteArray                    m_audioReadBuffer;
  quint64                       m_audioBytesReceived;
  quint64                       m_audioReadRateLastReported;

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
  QSharedPointer<QTcpSocket>    m_socket;
  QScopedPointer<QFile>         m_pcmOutputFile;
  QScopedPointer<QThread>       m_socketReader;
  // QScopedPointer<AudioSource>   m_audioSourceBuffer;
  QScopedPointer<QBuffer>       m_audioSourceBuffer;
  bool                          m_shouldBeConnected;
};

#endif // MAINWINDOW_H
