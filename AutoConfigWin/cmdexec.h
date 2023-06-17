#ifndef CMDEXEC_H
#define CMDEXEC_H

#include "constants.h"
#include <QObject>
#include <QProcess>

class CmdExec: public QObject
{
    Q_OBJECT

public:
    ExecRes exec_simple_command(std::wstring cmd_str);  //调用cmd窗口，使用同步的方式执行一项简单的命令。
    void exec_real_commands(RealCmdList opt_list);  //执行正式的命令
    void on_new_path(std::wstring path); //需要加入环境变量的路径列表。在执行时，会将其加入到环境变量中。
    void reset();

signals:
    void on_exec_success(std::wstring rtn_msg);
    void on_exec_failed(std::wstring err_msg);

public slots:
    // 进程执行完成和失败的调用内容
    void on_exec_finish(int exit_code, QProcess::ExitStatus exit_status);
    void on_exec_err(QProcess::ProcessError err_id);

private:
    std::vector<std::wstring> new_path_list;
    bool is_simple_command;
    QProcess proc;

public:
    CmdExec();
};

#endif // CMDEXEC_H
