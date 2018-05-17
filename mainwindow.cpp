#include "mainwindow.h"

#include <QAudioDeviceInfo>
#include <QAudioInput>

MainWindow::MainWindow()
  : m_socket(NULL)
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

  setLayout(mainLayout);
  setWindowTitle("Sound Test");
}

MainWindow::~MainWindow()
{
}


void
MainWindow::createServerGroupBox()
{
  QHBoxLayout* layout = new QHBoxLayout();

  m_serverGroupBox = new QGroupBox("Server");
  m_connectButton = new QPushButton("Connect");
  m_serverLineEdit = new QLineEdit("192.168.10.10");

  connect(m_connectButton, SIGNAL(released()), this, SLOT(connectButtonReleased()));

  layout->addWidget(m_connectButton);
  layout->addWidget(m_serverLineEdit);
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

  QAudioDeviceInfo const& defaultDeviceInfo = QAudioDeviceInfo::defaultInputDevice();
  m_audioInSelector->addItem(defaultDeviceInfo.deviceName(), qVariantFromValue(defaultDeviceInfo));
  for (auto& deviceInfo : QAudioDeviceInfo::availableDevices(QAudio::AudioInput))
  {
    if (deviceInfo != defaultDeviceInfo)
      m_audioInSelector->addItem(deviceInfo.deviceName(), qVariantFromValue(deviceInfo));
  }

  connect(m_audioInMuteButton, SIGNAL(released()), this, SLOT(audioInMuteButtonReleased()));

  hlayout->addWidget(m_audioInMuteButton);
  hlayout->addWidget(m_audioInProgressBar);
  vlayout->addLayout(hlayout);
  vlayout->addWidget(m_audioInSelector);
  m_audioInGroupBox->setLayout(vlayout);

}

void
MainWindow::createAudioOutGroupBox()
{
  QHBoxLayout* layout = new QHBoxLayout();
  m_audioOutGroupBox = new QGroupBox("Audio Out (From remote end)");
  m_audioOutMuteButton = new QPushButton("un-mute");
  m_audioOutProgressBar = new QProgressBar();
  m_audioOutProgressBar->setRange(0, 10);

  connect(m_audioOutMuteButton, SIGNAL(released()), this, SLOT(audioOutMuteButtonReleased()));

  layout->addWidget(m_audioOutMuteButton);
  layout->addWidget(m_audioOutProgressBar);
  m_audioOutGroupBox->setLayout(layout);
}

void
MainWindow::connectButtonReleased()
{
}

void
MainWindow::audioInMuteButtonReleased()
{
}

void
MainWindow::audioOutMuteButtonReleased()
{
}
