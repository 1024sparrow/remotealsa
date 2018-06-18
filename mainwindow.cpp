#include "mainwindow.h"
#include "logwindow.h"

#include <QAudioInput>
#include <QHostAddress>
#include <QSet>
#include <QStringBuilder>

static const qint32 kDefaultSamplingRate = 16000;
static const qint32 kDefaultNumberOfChannels = 2;

MainWindow::MainWindow()
  : m_serverGroupBox(nullptr)
  , m_connectButton(nullptr)
  , m_serverAddressLineEdit(nullptr)
  , m_serverPortLineEdit(nullptr)

  , m_audioInGroupBox(nullptr)
  , m_audioInMuteButton(nullptr)
  , m_audioInProgressBar(nullptr)
  , m_audioInSelector(nullptr)
  , m_audioInMute(true)
  , m_audioFromFile(false)
  , m_audioInDeviceInfo(QAudioDeviceInfo::defaultInputDevice())
  , m_audioInput()
  , m_audioInputDevice(nullptr)
  , m_audioWriteBuffer(32768, 0)
  , m_audioInFromDeviceRadioButton(nullptr)
  , m_audioInFromFileRadioButton(nullptr)
  , m_audioInOpenFilePushButton(nullptr)

  , m_audioOutGroupBox(nullptr)
  , m_audioOutMuteButton(nullptr)
  , m_audioOutProgressBar(nullptr)
  , m_audioOutSelector(nullptr)
  , m_audioOutMute(true)
  , m_audioOutput()
  , m_audioOutputFormat()
  , m_audioOutDevice(nullptr)
  , m_audioReadBuffer(32768, 0)
  , m_audioBytesReceived(0)
  , m_audioReadRateLastReported(QDateTime::currentMSecsSinceEpoch())

  , m_audioDecodeGroupBox(nullptr)
  , m_audioDecodeSampleRateLabel(nullptr)
  , m_audioDecodeSampleRateInput(nullptr)
  , m_audioDecodeChannelsLabel(nullptr)
  , m_audioDecodeChannelsInput(nullptr)
  , m_audioDecodeSampleSizeLabel(nullptr)
  , m_audioDecodeSampleSizeInput(nullptr)
  , m_audioDecodeCodecLabel(nullptr)
  , m_audioDecodeCodecInput(nullptr)
  , m_audioDecodeByteOrderLabel(nullptr)
  , m_audioDecodeByteOrderSelector(nullptr)
  , m_audioDecodeSampleTypeLabel(nullptr)
  , m_audioDecodeSampleTypeSelector(nullptr)

  , m_logWindow(nullptr)
  , m_dialogButtonBox(nullptr)

  , m_socket(nullptr)
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

  connect(&m_audioInputDecoder, &QAudioDecoder::bufferReady, [this]()
  {
    // TODO
  });
  connect(&m_audioInputDecoder, QOverload<QAudioDecoder::Error>::of(&QAudioDecoder::error),
         [this](QAudioDecoder::Error err)
  {
    qInfo() << "audio decoder error:" << err;
    qInfo() << m_audioInputDecoder.errorString();
  });
  connect(&m_audioInputDecoder, &QAudioDecoder::stateChanged,
         [this](QAudioDecoder::State newState)
  {
    qInfo() << "audio decoder state change:" << newState;
  });
  connect(&m_audioInputDecoder, &QAudioDecoder::finished, [this]()
  {
    // TODO
  });

  setLayout(mainLayout);
  setWindowTitle("Sound Test");
}

MainWindow::~MainWindow()
{
}

void
MainWindow::initializeAudioOutputDevice(QAudioDeviceInfo const& deviceInfo)
{
  qDebug() << "current deviceInfo:"  << deviceInfo.deviceName();
  m_logWindow->appendMessage(QString("Initialize audio out with device: %1").arg(deviceInfo.deviceName()));
  m_audioOutput.reset(new QAudioOutput(deviceInfo, getAudioOutputFormat()));
  m_audioOutDevice = m_audioOutput->start();
}

void
MainWindow::initializeAudioInputDevice(QAudioDeviceInfo const& deviceInfo)
{
  m_logWindow->appendMessage(QString("Initialize audio in with: %1").arg(deviceInfo.deviceName()));
  m_audioInput.reset(new QAudioInput(deviceInfo, getAudioInputFormat()));
  m_audioInputDevice = m_audioInput->start();
  connect(m_audioInputDevice, SIGNAL(readyRead()), this, SLOT(onIncomingSoundData()));
}

QAudioFormat
MainWindow::getAudioOutputFormat() const
{
  QAudioFormat format;
  format.setSampleRate(m_audioDecodeSampleRateInput->text().toInt());
  format.setChannelCount(m_audioDecodeChannelsInput->text().toInt());
  format.setSampleSize(m_audioDecodeSampleSizeInput->text().toInt());
  format.setCodec(m_audioDecodeCodecInput->text());
  format.setByteOrder(m_audioDecodeByteOrderSelector->currentData().value<QAudioFormat::Endian>());
  format.setSampleType(m_audioDecodeSampleTypeSelector->currentData().value<QAudioFormat::SampleType>());

  m_logWindow->appendMessage(QString("output format sample_rate:%1 channels:%2 sample_size:%3").
    arg(QString::number(format.sampleRate()), QString::number(format.channelCount()),
        QString::number(format.sampleSize())));
  m_logWindow->appendMessage(QString("output format codec:%1 byte_order:%2 sample_type:%3").
    arg(format.codec(), QString::number(format.byteOrder()), QString::number(format.sampleType())));

  return format;
}

QAudioFormat
MainWindow::getAudioInputFormat() const
{
  QAudioFormat format;
  format.setSampleRate(m_audioEncodeSampleRateInput->text().toInt());
  format.setChannelCount(m_audioEncodeChannelsInput->text().toInt());
  format.setSampleSize(m_audioEncodeSampleSizeInput->text().toInt());
  format.setCodec(m_audioEncodeCodecInput->text());
  format.setByteOrder(m_audioEncodeByteOrderSelector->currentData().value<QAudioFormat::Endian>());
  format.setSampleType(m_audioEncodeSampleTypeSelector->currentData().value<QAudioFormat::SampleType>());

  m_logWindow->appendMessage(QString("input format sample_rate:%1 channels:%2 sample_size:%3").
    arg(QString::number(format.sampleRate()), QString::number(format.channelCount()),
        QString::number(format.sampleSize())));
  m_logWindow->appendMessage(QString("input format codec:%1 byte_order:%2 sample_type:%3").
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
  createAudioEncodeFormatGroupBox();

  QVBoxLayout* vlayout = new QVBoxLayout();
  QHBoxLayout* hlayout = new QHBoxLayout();
  QHBoxLayout* pushButtonLayout = new QHBoxLayout();

  m_audioInFromDeviceRadioButton = new QRadioButton("From Device");
  m_audioInFromDeviceRadioButton->setChecked(true);
  m_audioInFromFileRadioButton = new QRadioButton("From File");
  m_audioInFromFileRadioButton->setChecked(false);
  m_audioInOpenFilePushButton = new QPushButton("Open File");
  m_audioInOpenFilePushButton->setEnabled(false);


  m_audioInGroupBox = new QGroupBox("Audio In (From this computer)");
  m_audioInMuteButton = new QPushButton("un-mute");
  m_audioInProgressBar = new QProgressBar();
  m_audioInProgressBar->setRange(0, 10);
  m_audioInSelector = new QComboBox();
  m_audioEncodeVolumeSlider = new QSlider();
  m_audioEncodeVolumeSlider->setOrientation(Qt::Horizontal);
  m_audioEncodeVolumeSlider->setRange(0, 100);
  m_audioEncodeVolumeSlider->setValue(60);

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

  connect(m_audioInOpenFilePushButton, &QPushButton::released, [this]()
  {
    QString wavFile = QFileDialog::getOpenFileName(this, "Open WAV File");
    m_audioInputDecoder.setAudioFormat(getAudioInputFormat());
    m_audioInputDecoder.setSourceFilename(wavFile);
    m_audioInputDecoder.start();
  });

  connect(m_audioInFromDeviceRadioButton, &QRadioButton::toggled, [this]()
  {
    m_audioFromFile = false;
    m_audioInOpenFilePushButton->setEnabled(false);
    m_audioInSelector->setEnabled(true);
  });

  connect(m_audioInFromFileRadioButton, &QRadioButton::toggled, [this]()
  {
    m_audioFromFile = true;
    m_audioInOpenFilePushButton->setEnabled(true);
    m_audioInSelector->setEnabled(false);
  });

  connect(m_audioInMuteButton, SIGNAL(released()), this, SLOT(audioInMuteButtonReleased()));
  connect(m_audioInSelector, SIGNAL(activated(int)), this, SLOT(audioInDeviceChanged(int)));
  connect(m_audioEncodeVolumeSlider, SIGNAL(valueChanged(int)), this, SLOT(onEncodeVolumeValueChanged(int)));

  pushButtonLayout->addWidget(m_audioInFromDeviceRadioButton);
  pushButtonLayout->addWidget(m_audioInFromFileRadioButton);
  pushButtonLayout->addWidget(m_audioInOpenFilePushButton);
  vlayout->addLayout(pushButtonLayout);

  hlayout->addWidget(m_audioInMuteButton);
  hlayout->addWidget(m_audioInProgressBar);
  vlayout->addLayout(hlayout);

  vlayout->addWidget(m_audioInSelector);
  vlayout->addWidget(m_audioEncodeVolumeSlider);
  vlayout->addWidget(m_audioEncodeGroupBox);

  m_audioInGroupBox->setLayout(vlayout);
}

void
MainWindow::createAudioOutGroupBox()
{
  createAudioDecodeFormatGroupBox();

  QVBoxLayout* vlayout = new QVBoxLayout();
  QHBoxLayout* hlayout = new QHBoxLayout();
  m_audioOutGroupBox = new QGroupBox("Audio Out (From camera)");
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
  m_audioDecodeSampleRateInput = new QLineEdit(QString::number(kDefaultSamplingRate));
  gridLayout->addWidget(m_audioDecodeSampleRateLabel, 0, 0, 1, 1);
  gridLayout->addWidget(m_audioDecodeSampleRateInput, 0, 1, 1, 1);

  m_audioDecodeChannelsLabel = new QLabel("Channels");
  m_audioDecodeChannelsInput = new QLineEdit(QString::number(kDefaultNumberOfChannels));
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
MainWindow::createAudioEncodeFormatGroupBox()
{
  m_audioEncodeGroupBox = new QGroupBox("Encode Parameters");
  QGridLayout* gridLayout = new QGridLayout();

  m_audioEncodeSampleRateLabel = new QLabel("Sample Rate");
  m_audioEncodeSampleRateInput = new QLineEdit(QString::number(kDefaultSamplingRate));
  gridLayout->addWidget(m_audioEncodeSampleRateLabel, 0, 0, 1, 1);
  gridLayout->addWidget(m_audioEncodeSampleRateInput, 0, 1, 1, 1);

  m_audioEncodeChannelsLabel = new QLabel("Channels");
  m_audioEncodeChannelsInput = new QLineEdit(QString::number(kDefaultNumberOfChannels));
  gridLayout->addWidget(m_audioEncodeChannelsLabel, 0, 2, 1, 1);
  gridLayout->addWidget(m_audioEncodeChannelsInput, 0, 3, 1, 1);

  m_audioEncodeSampleSizeLabel = new QLabel("Sample Size");
  m_audioEncodeSampleSizeInput = new QLineEdit("16");
  gridLayout->addWidget(m_audioEncodeSampleSizeLabel, 1, 0, 1, 1);
  gridLayout->addWidget(m_audioEncodeSampleSizeInput, 1, 1, 1, 1);

  m_audioEncodeCodecLabel = new QLabel("Codec");
  m_audioEncodeCodecInput = new QLineEdit("audio/pcm");
  m_audioEncodeCodecInput->setReadOnly(true);
  gridLayout->addWidget(m_audioEncodeCodecLabel, 1, 2, 1, 1);
  gridLayout->addWidget(m_audioEncodeCodecInput, 1, 3, 1, 1);

  m_audioEncodeByteOrderLabel = new QLabel("Byte Order");
  m_audioEncodeByteOrderSelector = new QComboBox();
  m_audioEncodeByteOrderSelector->addItem("Little Endian", qVariantFromValue(QAudioFormat::LittleEndian));
  m_audioEncodeByteOrderSelector->addItem("Big Endian", qVariantFromValue(QAudioFormat::BigEndian));
  gridLayout->addWidget(m_audioEncodeByteOrderLabel, 2, 0, 1, 1);
  gridLayout->addWidget(m_audioEncodeByteOrderSelector, 2, 1, 1, 1);

  m_audioEncodeSampleTypeLabel = new QLabel("Sample Type");
  m_audioEncodeSampleTypeSelector = new QComboBox();
  m_audioEncodeSampleTypeSelector->addItem("SignedInt", qVariantFromValue(QAudioFormat::SignedInt));
  m_audioEncodeSampleTypeSelector->addItem("UnSignedInt", qVariantFromValue(QAudioFormat::UnSignedInt));
  m_audioEncodeSampleTypeSelector->addItem("Float", qVariantFromValue(QAudioFormat::Float));
  gridLayout->addWidget(m_audioEncodeSampleTypeLabel, 2, 2, 1, 1);
  gridLayout->addWidget(m_audioEncodeSampleTypeSelector, 2, 3, 1, 1);

  m_audioEncodeGroupBox->setLayout(gridLayout);
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
    initializeAudioOutputDevice(m_audioOutSelector->currentData().value<QAudioDeviceInfo>());
    initializeAudioInputDevice(m_audioInSelector->currentData().value<QAudioDeviceInfo>());

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
    m_logWindow->appendMessage("audio output un-muted");

    QString tempPath = QDir().tempPath() + "/out.pcm";
    // m_pcmOutputFile.reset(new QFile(tempPath));
    // m_pcmOutputFile->open(QFile::WriteOnly | QFile::Truncate);
    m_logWindow->appendMessage(QString("opened for PCM capture:%1").arg(tempPath));
  }
  else
  {
    if (m_pcmOutputFile)
      m_pcmOutputFile.reset();

    m_audioOutMuteButton->setText("un-mute");
    m_audioOutMute = true;
    m_logWindow->appendMessage("audio output muted");
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
  static qint64 const periodSize = m_audioOutput->periodSize();

  int chunks = m_audioOutput->bytesFree() / periodSize;
  while (chunks) {

    qint64 len = m_socket->peek(m_audioReadBuffer.data(), periodSize);
    if (len < periodSize)
      return;

    m_socket->read(m_audioReadBuffer.data(), len);

    if (!m_audioOutMute)
    {
      m_audioOutDevice->write(m_audioReadBuffer.data(), len);
      if (m_pcmOutputFile)
        m_pcmOutputFile->write(m_audioReadBuffer.data(), len);
    }

    --chunks;
  }
}


void
MainWindow::onIncomingSoundData()
{
  if (m_audioFromFile)
    return;

  qint64 bytes_ready = m_audioInput->bytesReady();
  qint64 bytes_read = m_audioInputDevice->read(m_audioWriteBuffer.data(), bytes_ready);

  if (m_socket && !m_audioInMute)
    m_socket->write(m_audioWriteBuffer.constData(), bytes_read);
}

void
MainWindow::onSocketError(QAbstractSocket::SocketError /*socketError*/)
{
  m_logWindow->appendMessage(QString("socket error: %1").arg(m_socket->errorString()));
}

void
MainWindow::audioInDeviceChanged(int index)
{
  m_audioInput->stop();
  initializeAudioOutputDevice(m_audioInSelector->itemData(index).value<QAudioDeviceInfo>());
}

void
MainWindow::audioOutDeviceChanged(int index)
{
  m_audioOutput->stop();
  initializeAudioOutputDevice(m_audioOutSelector->itemData(index).value<QAudioDeviceInfo>());
}

void
MainWindow::onEncodeVolumeValueChanged(int value)
{
  qreal n;
 
#if QT_VERSION < QT_VERSION_CHECK(5, 8, 0)
  n = (qreal(value) / 100);
#else
  n = QAudio::convertVolume(value / qreal(100),
    QAudio::LogarithmicVolumeScale, QAudio::LinearVolumeScale);
#endif

  if (m_audioInput)
    m_audioInput->setVolume(n);

  if (m_audioOutput)
    m_audioOutput->stop();
}
