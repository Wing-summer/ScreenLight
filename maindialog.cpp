#include "maindialog.h"
#include <DMessageManager>
#include <DTitlebar>
#include <QApplication>
#include <QCloseEvent>
#include <QFormLayout>
#include <QSettings>
#include <QVBoxLayout>

#include <systemd/sd-bus.h>

MainDialog::MainDialog(QList<BackLightInfo> &infos) : buffer(infos) {
  auto title = titlebar();
  title->setTitle(tr("ScreenLight"));
  setWindowFlag(Qt::WindowMaximizeButtonHint, false);
  QIcon icon(":/images/logo.svg");
  setWindowIcon(icon);

  setMinimumWidth(500);
  auto w = new QWidget(this);
  setCentralWidget(w);
  auto layout = new QVBoxLayout(w);

  layout->addWidget(new DLabel(tr("SelectToSet"), this), 0, Qt::AlignCenter);
  layout->addSpacing(5);

  QStringList backlights;
  for (auto &item : infos) {
    backlights.append(item.name);
  }

  combox = new DComboBox(this);
  combox->addItems(backlights);
  combox->setCurrentIndex(0);
  layout->addWidget(combox);
  layout->addSpacing(10);

  svlbl = new DLabel(this);
  layout->addWidget(svlbl, 0, Qt::AlignCenter);

  slider = new DSlider(Qt::Horizontal, this);
  slider->setMaximum(100);
  slider->setMinimum(1);
  slider->setPageStep(5);
  slider->setIconSize(QSize(25, 25));
  slider->setLeftIcon(icon);
  auto fir = infos[0];
  slider->setValue(fir.curBrightness * 100 / fir.maxBrightness);
  svlbl->setText(QString::number(slider->value()) + " %");
  layout->addWidget(slider);

  layout->addSpacing(10);
  layout->addWidget(
      new DLabel(QString(500 / fontMetrics().horizontalAdvance('-'), '-'),
                 this),
      0, Qt::AlignCenter);
  layout->addSpacing(5);
  auto flayout = new QFormLayout;
  flayout->setMargin(5);
  layout->addLayout(flayout);

  spin = new DSpinBox(this);
  spin->setRange(1, 20);
  spin->setSuffix(" %");
  spin->setValue(5);
  spin->setToolTip(tr("Range1to20"));
  flayout->addRow(tr("Interval"), spin);

  hkLighter = new QHotkey(this);
  seqLighter = new DKeySequenceEdit(this);
  flayout->addRow(tr("Lighter"), seqLighter);
  connect(seqLighter, &DKeySequenceEdit::keySequenceChanged, this,
          [=](const QKeySequence &keySequence) {
            if (!hkLighter->setShortcut(keySequence, true)) {
              DMessageManager::instance()->sendMessage(
                  this, icon, tr("HotkeyErr:") + keySequence.toString());
              seqLighter->clear();
            }
          });
  connect(hkLighter, &QHotkey::activated, this, [=] {
    auto interval = spin->value();
    slider->setValue(slider->value() + interval);
  });

  hkDaker = new QHotkey(this);
  seqDarker = new DKeySequenceEdit(this);
  flayout->addRow(tr("Darker"), seqDarker);
  connect(seqDarker, &DKeySequenceEdit::keySequenceChanged, this,
          [=](const QKeySequence &keySequence) {
            if (!hkDaker->setShortcut(keySequence, true)) {
              DMessageManager::instance()->sendMessage(
                  this, icon, tr("HotkeyErr:") + keySequence.toString());
              seqDarker->clear();
            }
          });
  connect(hkDaker, &QHotkey::activated, this, [=] {
    auto interval = spin->value();
    slider->setValue(slider->value() - interval);
  });

  connect(combox, QOverload<int>::of(&DComboBox::currentIndexChanged), this,
          [=](int index) {
            curindex = index;
            auto &fir = buffer[index];
            slider->setValue(fir.curBrightness * 100 / fir.maxBrightness);
          });
  connect(slider, &DSlider::valueChanged, this, [=](int value) {
    auto &fir = buffer[curindex];
    fir.curBrightness = value * fir.maxBrightness / 100;
    if (!this->setBrightness(fir.curBrightness)) {
      DMessageManager::instance()->sendMessage(this, icon,
                                               tr("SetBrightnessError"));
    }
    svlbl->setText(QString::number(value) + " %");
  });

  auto menu = new DMenu(this);
  aLighter = new QAction(tr("EnabledLighter"), menu);
  aLighter->setCheckable(true);
  connect(aLighter, &QAction::toggled, this,
          [=](bool v) { hkLighter->setRegistered(v); });
  menu->addAction(aLighter);
  aDarker = new QAction(tr("EnabledDarker"), menu);
  aDarker->setCheckable(true);
  connect(aDarker, &QAction::toggled, this,
          [=](bool v) { hkDaker->setRegistered(v); });
  menu->addAction(aDarker);
  title->setMenu(menu);
  loadSettings();
}

bool MainDialog::setBrightness(int value) {
  sd_bus *bus = nullptr;
  int r = sd_bus_default_system(&bus);
  if (r < 0) {
    fprintf(stderr, "Can't connect to system bus: %s\n", strerror(-r));
    return false;
  }

  r = sd_bus_call_method(
      bus, "org.freedesktop.login1", "/org/freedesktop/login1/session/auto",
      "org.freedesktop.login1.Session", "SetBrightness", nullptr, nullptr,
      "ssu", "backlight", combox->currentText().toLocal8Bit().constData(),
      value);
  if (r < 0)
    fprintf(stderr, "Failed to set brightness: %s\n", strerror(-r));

  sd_bus_unref(bus);

  return r >= 0;
}

void MainDialog::loadSettings() {
  QSettings settings(QApplication::organizationName(),
                     QApplication::applicationName());
  spin->setValue(settings.value("Interval", 5).toInt());
  seqLighter->setKeySequence(
      settings
          .value("HkLighter", QKeySequence(Qt::ControlModifier |
                                           Qt::AltModifier | Qt::UpArrow))
          .value<QKeySequence>());
  seqDarker->setKeySequence(
      settings
          .value("HkDarker", QKeySequence(Qt::ControlModifier |
                                          Qt::AltModifier | Qt::DownArrow))
          .value<QKeySequence>());
  aLighter->setChecked(settings.value("EnLighter", true).value<bool>());
  aDarker->setChecked(settings.value("EnDarker", true).value<bool>());
}

void MainDialog::saveSettings() {
  QSettings settings(QApplication::organizationName(),
                     QApplication::applicationName());
  settings.setValue("Interval", spin->value());
  settings.setValue("HkLighter", hkLighter->shortcut());
  settings.setValue("HkDarker", hkDaker->shortcut());
  settings.setValue("EnLighter", aLighter->isChecked());
  settings.setValue("EnDarker", aDarker->isChecked());
}

void MainDialog::setScreenBrightness(int percent) { slider->setValue(percent); }

void MainDialog::closeEvent(QCloseEvent *event) {
  event->ignore();
  hide();
  saveSettings();
}
