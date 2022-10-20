#ifndef MAINDIALOG_H
#define MAINDIALOG_H

#include "QHotkey/qhotkey.h"
#include "common.h"
#include <DComboBox>
#include <DKeySequenceEdit>
#include <DLabel>
#include <DMainWindow>
#include <DSlider>
#include <DSpinBox>

DWIDGET_USE_NAMESPACE

class MainDialog : public DMainWindow {
  Q_OBJECT
public:
  MainDialog(QList<BackLightInfo> &infos);

private:
  bool setBrightness(int value);
  void loadSettings();

public:
  void saveSettings();
  void setScreenBrightness(int percent);

protected:
  void closeEvent(QCloseEvent *event) override;

private:
  QHotkey *hkLighter, *hkDaker;
  DKeySequenceEdit *seqLighter, *seqDarker;
  QAction *aLighter, *aDarker;

private:
  QList<BackLightInfo> &buffer;
  int curindex = 0;

  DComboBox *combox;
  DSlider *slider;
  DSpinBox *spin;

  DLabel *svlbl;
};

#endif // MAINDIALOG_H
