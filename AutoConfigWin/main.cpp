#include "frontend/mainwindow.h"
#include <vector>
#include <QApplication>
#include <thread>

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
    MainWindow w(&gpt_comm, &cmd_exec);
    w.show();

    return a.exec();
}
