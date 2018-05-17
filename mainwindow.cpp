#include "mainwindow.h"

#include <QAudioInput>
#include <QHostAddress>
#include <QSet>

MainWindow::MainWindow()
  : m_socket(NULL)
  , m_audioInMute(true)
  , m_audioOutMute(true)
  , m_audioInDeviceInfo(QAudioDeviceInfo::defaultInputDevice())
  , m_audioReadBuffer(32768, 0)
{
  createServerGroupBox();
  createAudioInGroupBox();
  createAudioOutGroupBox();

  m_feedbackText = new QTextEdit();
  m_dialogButtonBox = new QDialogButtonBox(QDialogButtonBox::Cancel);
  connect(m_dialogButtonBox, SIGNAL(rejected()), this, SLOT(reject()));

  QVBoxLayout* mainLayout = new QVBoxLayout();
  mainLayout->addWidget(m_serverGroupBox);
  mainLayout->addWidget(m_audioInGroupBox);
  mainLayout->addWidget(m_audioOutGroupBox);
  mainLayout->addWidget(m_feedbackText);
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
  QAudioFormat format;
  format.setSampleRate(44100);
  format.setChannelCount(2);
  format.setSampleSize(16);
  format.setCodec("audio/pcm");
  format.setByteOrder(QAudioFormat::LittleEndian);
  format.setSampleType(QAudioFormat::SignedInt);

  m_audioOutput.reset(new QAudioOutput(deviceInfo, format));
  m_audioOutDevice = m_audioOutput->start();
  qDebug() << "output device started";
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
    qDebug() << "input audioDeviceInfo:" << deviceInfo.deviceName();
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
    qDebug() << "output audioDeviceInfo:" << deviceInfo.deviceName();
    if ((deviceInfo != defaultDeviceInfo) && !set.contains(deviceInfo.deviceName()))
    {
      m_audioOutSelector->addItem(deviceInfo.deviceName(), qVariantFromValue(deviceInfo));
      set.insert(deviceInfo.deviceName());
    }
  }

  initializeAudio(defaultDeviceInfo);

  connect(m_audioOutMuteButton, SIGNAL(released()), this, SLOT(audioOutMuteButtonReleased()));
  connect(m_audioOutSelector, SIGNAL(activated(int)), this, SLOT(audioOutDeviceChanged(int)));

  hlayout->addWidget(m_audioOutMuteButton);
  hlayout->addWidget(m_audioOutProgressBar);
  vlayout->addLayout(hlayout);
  vlayout->addWidget(m_audioOutSelector);
  m_audioOutGroupBox->setLayout(vlayout);
}

void
MainWindow::connectButtonReleased()
{
  if (m_socket != nullptr)
  {
    m_socket->close();
    delete m_socket;
    m_connectButton->setText("Connect");
  }
  else
  {
    m_socket = new QTcpSocket();
    connect(m_socket, SIGNAL(connected()), this, SLOT(onSocketConnected()));
    connect(m_socket, SIGNAL(readyRead()), this, SLOT(onSocketReadyRead()));
    m_socket->connectToHost(m_serverAddressLineEdit->text(), m_serverPortLineEdit->text().toUInt());
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
  }
  else
  {
    m_audioInMuteButton->setText("un-mute");
    m_audioInMute = true;
  }
}

void
MainWindow::audioOutMuteButtonReleased()
{
  if (m_audioOutMute)
  {
    m_audioOutMuteButton->setText("mute");
    m_audioOutMute = false;
  }
  else
  {
    m_audioOutMuteButton->setText("un-mute");
    m_audioOutMute = true;
  }
}

void
MainWindow::onSocketConnected()
{
  qDebug() << "connected to:" << m_socket->peerAddress()
           << ":" << m_socket->peerPort();
}

void
MainWindow::onSocketReadyRead()
{
  //qDebug() << "write:" << buff.size();
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
MainWindow::audioInDeviceChanged(int index)
{
  qDebug() << "audioInDeviceChanged:" << index;
}

void
MainWindow::audioOutDeviceChanged(int index)
{
  qDebug() << "audioOutDeviceChanged:" << index;

  m_audioOutput->stop();
  initializeAudio(m_audioOutSelector->itemData(index).value<QAudioDeviceInfo>());
}
