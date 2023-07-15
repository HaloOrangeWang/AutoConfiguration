#include "frontend/mainwindow.h"
#include <vector>
#include <QApplication>
#include <thread>

MainWindow* w;

void textedit_log_handler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    w->textedit_log_handler(type, context, msg);
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    qRegisterMetaType<std::string>("std::string");
    qRegisterMetaType<std::wstring>("std::wstring");
    qRegisterMetaType<InstallList>("InstallList");
    qRegisterMetaType<ConfirmInstallRes>("ConfirmInstallRes");
    qRegisterMetaType<ReplaceList>("ReplaceList");
    qRegisterMetaType<ConfirmReplaceRes>("ConfirmReplaceRes");
    qRegisterMetaType<PathList>("PathList");
    qRegisterMetaType<ConfirmPathRes>("ConfirmPathRes");
    qRegisterMetaType<Opt>("Opt");

    GptComm gpt_comm;
    CmdExec cmd_exec;
    w = new MainWindow(&gpt_comm, &cmd_exec);

    // 将日志输出重定向至界面控件中
    qInstallMessageHandler(&textedit_log_handler);

    w->show();

    return a.exec();
}
