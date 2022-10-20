#include <DAboutDialog>
#include <DApplication>
#include <DApplicationSettings>
#include <DMainWindow>
#include <DMessageBox>
#include <DTitlebar>
#include <DWidgetUtil>
#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QSystemTrayIcon>
#include <QtDBus>

#include "common.h"
#include "maindialog.h"

DWIDGET_USE_NAMESPACE
DCORE_USE_NAMESPACE

int main(int argc, char *argv[]) {
  //解决 root/ubuntu 主题样式走形
  qputenv("XDG_CURRENT_DESKTOP", "Deepin");
  QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

  // 程序内强制添加 -platformtheme
  // deepin 参数喂给 Qt 让 Qt 正确使用 Deepin 主题修复各种奇怪样式问题
  QVector<char *> fakeArgs(argc + 2);
  char fa1[] = "-platformtheme";
  char fa2[] = "deepin";
  fakeArgs[0] = argv[0];
  fakeArgs[1] = fa1;
  fakeArgs[2] = fa2;

  for (int i = 1; i < argc; i++)
    fakeArgs[i + 2] = argv[i];
  int fakeArgc = argc + 2;

  QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
  DApplication a(fakeArgc, fakeArgs.data());
  QCoreApplication::setAttribute(Qt::AA_UseOpenGLES);

  auto s = a.applicationDirPath() + "/lang/default.qm";
  QTranslator translator;
  if (!translator.load(s)) {
    DMessageBox::critical(nullptr, "Error", "Error Loading Translation File!",
                          DMessageBox::Ok);
    return -1;
  }
  a.installTranslator(&translator);

  a.setOrganizationName("WingCloud");
  a.setApplicationName(QObject::tr("ScreenLight"));
  a.setApplicationVersion("1.5.1");
  a.setApplicationLicense("AGPL-3.0");
  auto icon = QIcon(":/images/logo.svg");
  a.setProductIcon(icon);
  a.setProductName(QObject::tr("ScreenLight"));
  a.setApplicationDescription(
      QObject::tr("a tool to change the brightness of screen on Deepin"));
  a.setVisibleMenuShortcutText(true);

  a.loadTranslator();
  a.setApplicationDisplayName("ScreenLight");

  if (!a.setSingleInstance("com.Wingsummer.ScreenLight")) {
    return -1;
  }

  // 保存程序的窗口主题设置
  DApplicationSettings as;
  Q_UNUSED(as)

  // 开始解析数据
  QDir display("/sys/class/backlight");

  QList<BackLightInfo> infos;

  for (auto &item : display.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
    BackLightInfo info;
    info.name = item.fileName();
    QFile cf(item.absoluteFilePath() + "/brightness");
    if (cf.open(QFile::ReadOnly)) {
      info.curBrightness = cf.readAll().toInt();
      cf.close();
    } else {
      continue;
    }

    QFile f(item.absoluteFilePath() + "/max_brightness");
    if (f.open(QFile::ReadOnly)) {
      info.maxBrightness = f.readAll().toInt();
      f.close();
    } else {
      continue;
    }

    infos.append(info);
  }

  // 初始化托盘

  MainDialog w(infos);
  if (argc - 1 && !strcmp(argv[1], "show")) {
    w.show();
  }

  Dtk::Widget::moveToCenter(&w);

  // 单例传参
  auto instance = DGuiApplicationHelper::instance();
  QObject::connect(instance, &DGuiApplicationHelper::newProcessInstance,
                   [&](qint64 pid, const QStringList &arguments) {
                     Q_UNUSED(pid);
                     Q_UNUSED(arguments);
                     w.show();
                     w.activateWindow();
                     w.raise();
                   });

  QSystemTrayIcon systray(icon);
  QMenu sysmenu;
  auto menu = &sysmenu;

  auto ac = new QAction(QObject::tr("ShowMain"), menu);
  QObject::connect(ac, &QAction::triggered, [&] {
    w.show();
    w.activateWindow();
    w.raise();
  });
  sysmenu.addAction(ac);
  sysmenu.addSeparator();

  auto m = new QMenu(QObject::tr("ShortCut"), menu);
  auto file = a.applicationDirPath() + "/sc.txt";
  if (QFile::exists(file)) {
    QFile f(file);
    if (f.open(QFile::ReadOnly)) {
      while (!f.atEnd()) {
        auto buf = f.readLine().trimmed();
        if (buf.isEmpty() || buf.startsWith('#'))
          continue;
        bool b;
        auto p = buf.toInt(&b);
        if (b && p > 0 && p <= 100) {
          ac = new QAction(m);
          ac->setText(QString::number(p) + " %");
          QObject::connect(ac, &QAction::triggered,
                           [&w, p] { w.setScreenBrightness(p); });
          m->addAction(ac);
        }
      }
    }
  } else {
    ac = new QAction(m);
    ac->setText(QObject::tr("None"));
    ac->setEnabled(false);
    m->addAction(ac);
  }
  sysmenu.addMenu(m);

  ac = new QAction(QObject::tr("Help"), menu);
  QObject::connect(ac, &QAction::triggered, [=] {
    QDesktopServices::openUrl(QUrl("https://code.gitlink.org.cn/wingsummer/"
                                   "ScreenLight/wiki/%E6%96%87%E6%A1%A3"));
  });
  sysmenu.addAction(ac);

  ac = new QAction(QObject::tr("Exit"), menu);
  sysmenu.addAction(ac);

  systray.setToolTip(QObject::tr("ScreenLight"));
  systray.setContextMenu(menu);

  QObject::connect(ac, &QAction::triggered, [&w] {
    if (DMessageBox::question(&w, QObject::tr("Exit"),
                              QObject::tr("ConfirmExit")) == DMessageBox::Yes) {
      w.saveSettings();
      QApplication::exit(0);
    }
  });
  QObject::connect(&systray, &QSystemTrayIcon::activated,
                   [&w](QSystemTrayIcon::ActivationReason reason) {
                     if (reason == QSystemTrayIcon::ActivationReason::Trigger) {
                       w.show();
                       w.activateWindow();
                       w.raise();
                     }
                   });
  systray.show();

  return a.exec();
}
