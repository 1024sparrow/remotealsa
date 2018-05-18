#include "mainwindow.h"
#include "logwindow.h"

#include <QAudioInput>
#include <QHostAddress>
#include <QSet>
#include <QStringBuilder>

MainWindow::MainWindow()
  : m_socket(nullptr)
  , m_audioInMute(true)
  , m_audioOutMute(true)
  , m_audioInDeviceInfo(QAudioDeviceInfo::defaultInputDevice())
  , m_audioReadBuffer(32768, 0)
  , m_logWindow(nullptr)
  , m_bytesReceived(0)
{
  createServerGroupBox();
  createAudioInGroupBox();
  createAudioOutGroupBox();

  m_logWindow = new LogWindow();
  m_dialogButtonBox = new QDialogButtonBox(QDialogButtonBox::Cancel);
  connect(m_dialogButtonBox, SIGNAL(rejected()), this, SLOT(reject()));

  QVBoxLayout* mainLayout = new QVBoxLayout();
  mainLayout->addWidget(m_serverGroupBox);
  mainLayout->addWidget(m_audioInGroupBox);
  mainLayout->addWidget(m_audioOutGroupBox);
  mainLayout->addWidget(m_logWindow);
  mainLayout->addWidget(m_dialogButtonBox);

  //m_audioOutput.reset(new QAudioOutput()

  setLayout(mainLayout);
  setWindowTitle("Sound Test");
}

MainWindow::~MainWindow()
{
}

void
MainWindow::initializeAudio(QAudioDeviceInfo const& deviceInfo)
{
  qDebug() << "current deviceInfo:"  << deviceInfo.deviceName();
  m_logWindow->appendMessage(QString("Initialize audio with device: %1").arg(deviceInfo.deviceName()));
  m_audioOutput.reset(new QAudioOutput(deviceInfo, getAudioFormat()));
  m_audioOutDevice = m_audioOutput->start();
}

QAudioFormat
MainWindow::getAudioFormat() const
{
  QAudioFormat format;
  format.setSampleRate(m_audioDecodeSampleRateInput->text().toInt());
  format.setChannelCount(m_audioDecodeChannelsInput->text().toInt());
  format.setSampleSize(m_audioDecodeSampleSizeInput->text().toInt());
  format.setCodec(m_audioDecodeCodecInput->text());
  format.setByteOrder(m_audioDecodeByteOrderSelector->currentData().value<QAudioFormat::Endian>());
  format.setSampleType(m_audioDecodeSampleTypeSelector->currentData().value<QAudioFormat::SampleType>());

  m_logWindow->appendMessage(QString("format sample_rate:%1 channels:%2 sample_size:%3").
    arg(QString::number(format.sampleRate()), QString::number(format.channelCount()),
        QString::number(format.sampleSize())));
  m_logWindow->appendMessage(QString("format codec:%1 byte_order:%2 sample_type:%3").
    arg(format.codec(), QString::number(format.byteOrder()), QString::number(format.sampleType())));

  return format;
}

void
MainWindow::createServerGroupBox()
{
  QHBoxLayout* layout = new QHBoxLayout();

  m_serverGroupBox = new QGroupBox("Server");
  m_connectButton = new QPushButton("Connect");
  m_serverAddressLineEdit = new QLineEdit("10.0.0.245");
  m_serverPortLineEdit = new QLineEdit("10001");

  connect(m_connectButton, SIGNAL(released()), this, SLOT(connectButtonReleased()));

  layout->addWidget(m_connectButton);
  layout->addWidget(m_serverAddressLineEdit);
  layout->addWidget(m_serverPortLineEdit);
  m_serverGroupBox->setLayout(layout);
}

void
MainWindow::createAudioInGroupBox()
{
  QVBoxLayout* vlayout = new QVBoxLayout();
  QHBoxLayout* hlayout = new QHBoxLayout();
  m_audioInGroupBox = new QGroupBox("Audio In (From your mic)");
  m_audioInMuteButton = new QPushButton("un-mute");
  m_audioInProgressBar = new QProgressBar();
  m_audioInProgressBar->setRange(0, 10);
  m_audioInSelector = new QComboBox();

  QSet<QString> set;
  QAudioDeviceInfo const& defaultDeviceInfo = QAudioDeviceInfo::defaultInputDevice();
  m_audioInSelector->addItem(defaultDeviceInfo.deviceName(), qVariantFromValue(defaultDeviceInfo));
  for (auto& deviceInfo : QAudioDeviceInfo::availableDevices(QAudio::AudioInput))
  {
    if ((deviceInfo != defaultDeviceInfo) && !set.contains(deviceInfo.deviceName()))
    {
      m_audioInSelector->addItem(deviceInfo.deviceName(), qVariantFromValue(deviceInfo));
      set.insert(deviceInfo.deviceName());
    }
  }

  connect(m_audioInMuteButton, SIGNAL(released()), this, SLOT(audioInMuteButtonReleased()));
  connect(m_audioInSelector, SIGNAL(activated(int)), this, SLOT(audioInDeviceChanged(int)));

  hlayout->addWidget(m_audioInMuteButton);
  hlayout->addWidget(m_audioInProgressBar);
  vlayout->addLayout(hlayout);
  vlayout->addWidget(m_audioInSelector);
  m_audioInGroupBox->setLayout(vlayout);
}

void
MainWindow::createAudioOutGroupBox()
{
  createAudioDecodeFormatGroupBox();

  QVBoxLayout* vlayout = new QVBoxLayout();
  QHBoxLayout* hlayout = new QHBoxLayout();
  m_audioOutGroupBox = new QGroupBox("Audio Out (From remote end)");
  m_audioOutMuteButton = new QPushButton("un-mute");
  m_audioOutProgressBar = new QProgressBar();
  m_audioOutProgressBar->setRange(0, 10);
  m_audioOutSelector = new QComboBox();

  QSet<QString> set;
  QAudioDeviceInfo const& defaultDeviceInfo = QAudioDeviceInfo::defaultOutputDevice();
  m_audioOutSelector->addItem(defaultDeviceInfo.deviceName(), qVariantFromValue(defaultDeviceInfo));
  for (auto& deviceInfo : QAudioDeviceInfo::availableDevices(QAudio::AudioOutput))
  {
    if ((deviceInfo != defaultDeviceInfo) && !set.contains(deviceInfo.deviceName()))
    {
      m_audioOutSelector->addItem(deviceInfo.deviceName(), qVariantFromValue(deviceInfo));
      set.insert(deviceInfo.deviceName());
    }
  }

  connect(m_audioOutMuteButton, SIGNAL(released()), this, SLOT(audioOutMuteButtonReleased()));
  connect(m_audioOutSelector, SIGNAL(activated(int)), this, SLOT(audioOutDeviceChanged(int)));

  hlayout->addWidget(m_audioOutMuteButton);
  hlayout->addWidget(m_audioOutProgressBar);
  vlayout->addLayout(hlayout);
  vlayout->addWidget(m_audioOutSelector);
  vlayout->addWidget(m_audioDecodeGroupBox);
  m_audioOutGroupBox->setLayout(vlayout);
}

void
MainWindow::createAudioDecodeFormatGroupBox()
{
  m_audioDecodeGroupBox = new QGroupBox("Decode Parameters");
  QGridLayout* gridLayout = new QGridLayout();

  m_audioDecodeSampleRateLabel = new QLabel("Sample Rate");
  m_audioDecodeSampleRateInput = new QLineEdit("44100");
  gridLayout->addWidget(m_audioDecodeSampleRateLabel, 0, 0, 1, 1);
  gridLayout->addWidget(m_audioDecodeSampleRateInput, 0, 1, 1, 1);

  m_audioDecodeChannelsLabel = new QLabel("Channels");
  m_audioDecodeChannelsInput = new QLineEdit("2");
  gridLayout->addWidget(m_audioDecodeChannelsLabel, 0, 2, 1, 1);
  gridLayout->addWidget(m_audioDecodeChannelsInput, 0, 3, 1, 1);

  m_audioDecodeSampleSizeLabel = new QLabel("Sample Size");
  m_audioDecodeSampleSizeInput = new QLineEdit("16");
  gridLayout->addWidget(m_audioDecodeSampleSizeLabel, 1, 0, 1, 1);
  gridLayout->addWidget(m_audioDecodeSampleSizeInput, 1, 1, 1, 1);

  m_audioDecodeCodecLabel = new QLabel("Codec");
  m_audioDecodeCodecInput = new QLineEdit("audio/pcm");
  m_audioDecodeCodecInput->setReadOnly(true);
  gridLayout->addWidget(m_audioDecodeCodecLabel, 1, 2, 1, 1);
  gridLayout->addWidget(m_audioDecodeCodecInput, 1, 3, 1, 1);

  m_audioDecodeByteOrderLabel = new QLabel("Byte Order");
  m_audioDecodeByteOrderSelector = new QComboBox();
  m_audioDecodeByteOrderSelector->addItem("Little Endian", qVariantFromValue(QAudioFormat::LittleEndian));
  m_audioDecodeByteOrderSelector->addItem("Big Endian", qVariantFromValue(QAudioFormat::BigEndian));
  gridLayout->addWidget(m_audioDecodeByteOrderLabel, 2, 0, 1, 1);
  gridLayout->addWidget(m_audioDecodeByteOrderSelector, 2, 1, 1, 1);

  m_audioDecodeSampleTypeLabel = new QLabel("Sample Type");
  m_audioDecodeSampleTypeSelector = new QComboBox();
  m_audioDecodeSampleTypeSelector->addItem("SignedInt", qVariantFromValue(QAudioFormat::SignedInt));
  m_audioDecodeSampleTypeSelector->addItem("UnSignedInt", qVariantFromValue(QAudioFormat::UnSignedInt));
  m_audioDecodeSampleTypeSelector->addItem("Float", qVariantFromValue(QAudioFormat::Float));
  gridLayout->addWidget(m_audioDecodeSampleTypeLabel, 2, 2, 1, 1);
  gridLayout->addWidget(m_audioDecodeSampleTypeSelector, 2, 3, 1, 1);

  m_audioDecodeGroupBox->setLayout(gridLayout);
}

void
MainWindow::connectButtonReleased()
{
  if (m_socket != nullptr)
  {
    m_logWindow->appendMessage("closing socket");

    m_socket->close();
    delete m_socket;
    m_socket = nullptr;
    m_connectButton->setText("Connect");
  }
  else
  {
    initializeAudio(m_audioOutSelector->currentData().value<QAudioDeviceInfo>());

    quint16 port = static_cast<quint16>(m_serverPortLineEdit->text().toUInt());
    QString host = m_serverAddressLineEdit->text();

    m_logWindow->appendMessage(QString("connecting to host %1:%2").arg(host, QString::number(port)));
    m_socket = new QTcpSocket();
    connect(m_socket, SIGNAL(connected()), this, SLOT(onSocketConnected()));
    connect(m_socket, SIGNAL(readyRead()), this, SLOT(onSocketReadyRead()));
    connect(m_socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(onSocketError(QAbstractSocket::SocketError)));
    m_socket->connectToHost(host, port);
    m_connectButton->setText("Disconnect");
  }
}

void
MainWindow::audioInMuteButtonReleased()
{
  if (m_audioInMute)
  {
    m_audioInMuteButton->setText("mute");
    m_audioInMute = false;
    m_logWindow->appendMessage("audio input muted");
  }
  else
  {
    m_audioInMuteButton->setText("un-mute");
    m_audioInMute = true;
    m_logWindow->appendMessage("audio input un-muted");
  }
}

void
MainWindow::audioOutMuteButtonReleased()
{
  if (m_audioOutMute)
  {
    m_audioOutMuteButton->setText("mute");
    m_audioOutMute = false;
    m_logWindow->appendMessage("audio output muted");
  }
  else
  {
    m_audioOutMuteButton->setText("un-mute");
    m_audioOutMute = true;
    m_logWindow->appendMessage("audio output un-muted");
  }
}

void
MainWindow::onSocketConnected()
{
  m_logWindow->appendMessage(QString("connected to %1:%2")
    .arg(m_socket->peerAddress().toString(), QString::number(m_socket->peerPort())));
}

void
MainWindow::onSocketReadyRead()
{
  int x = m_audioOutput->periodSize();

  int chunks = m_audioOutput->bytesFree() / m_audioOutput->periodSize();
  while (chunks) {
    qint64 const len = m_socket->read(m_audioReadBuffer.data(), m_audioOutput->periodSize());
    if (len && !m_audioOutMute)
      m_audioOutDevice->write(m_audioReadBuffer.data(), len);
    if (len != m_audioOutput->periodSize())
        break;
    -- chunks;
  }
}

void
MainWindow::onSocketError(QAbstractSocket::SocketError /*socketError*/)
{
  m_logWindow->appendMessage(QString("socket error: %1").arg(m_socket->errorString()));
}

void
MainWindow::audioInDeviceChanged(int index)
{
  // qDebug() << "audioInDeviceChanged:" << index;
}

void
MainWindow::audioOutDeviceChanged(int index)
{
  m_audioOutput->stop();
  initializeAudio(m_audioOutSelector->itemData(index).value<QAudioDeviceInfo>());
}
