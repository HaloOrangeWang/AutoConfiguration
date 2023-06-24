#pragma execution_character_set("utf-8")

#include "cmdexec.h"
#include "funcs.h"
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <functional>
#include <fstream>

CmdExec::CmdExec()
{
    // 创建临时指令所在的文件夹
    boost::filesystem::path dirname("./tmp_command");
    if (!boost::filesystem::is_directory(dirname)){
        if (!boost::filesystem::create_directories(dirname)){
            throw std::exception();
        }
    }

    // 绑定进程执行完成和失败的回调
    connect(&proc, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(on_exec_finish(int, QProcess::ExitStatus)));
    connect(&proc, SIGNAL(errorOccurred(QProcess::ProcessError)), this, SLOT(on_exec_err(QProcess::ProcessError)));
}

void CmdExec::on_exec_finish(int exit_code, QProcess::ExitStatus exit_status)
{
    // 不处理简单的命令
    if (is_simple_command){
        return;
    }
    // 返回值为0表示执行成功，为其他值表示执行失败
//    if (exit_code == 0){
//        QString tmp_msg(proc.readAllStandardOutput());
//        std::wstring msg = L""; //tmp_msg.toStdWString();
//        emit on_exec_success(msg);
//    }else{
//        std::wstring msg = L"执行失败，返回值为 ";
//        msg.append(std::to_wstring(exit_code));
//        emit on_exec_failed(msg);
//    }
}

void CmdExec::on_exec_err(QProcess::ProcessError err_id)
{
    // 不处理简单的命令
    if (is_simple_command){
        return;
    }
    // 返回执行失败的信号
//    std::wstring msg = L"进程启动失败，错误码为 ";
//    msg.append(std::to_wstring(err_id));
//    emit on_exec_failed(msg);
}

ExecRes CmdExec::exec_simple_command(std::wstring cmd_str)
{
    // 将标志位修改为 0，不再弹出新的命令行窗口
    proc.setCreateProcessArgumentsModifier(nullptr);

    is_simple_command = true;
    proc.start(QString::fromStdWString(cmd_str));
    if (!proc.waitForStarted()){
        std::wstring cmd_str2 = L"cmd /c ";
        cmd_str2.append(cmd_str);
        proc.start(QString::fromStdWString(cmd_str2));
        if (!proc.waitForStarted()){
            return ExecRes{1, L""};
        }
    }
    if (!proc.waitForFinished()){
        return ExecRes{1, L""};
    }
    QString output(proc.readAllStandardOutput());
    QString err_msg(proc.readAllStandardError());
    std::wstring output_str = output.toStdWString();
    int res = proc.exitCode();

    return ExecRes{res, output_str};
}

void CmdExec::exec_real_commands(RealCmdList opt_list)
{
    // 执行时打开cmd窗口
    proc.setCreateProcessArgumentsModifier([](QProcess::CreateProcessArguments *args) {
        args->flags |= CREATE_NEW_CONSOLE;
        args->startupInfo->dwFlags &= ~STARTF_USESTDHANDLES;
    });

    is_simple_command = false;
    std::basic_ofstream<wchar_t> ofs;
    ofs.open("tmp_command/main.bat", std::ios::out);
    if (!ofs.is_open()){
        throw std::exception();
    }
    ofs << L"chcp 65001" << std::endl;

    std::function<void()> write_check = [&ofs](){
        ofs << L"\nif %errorlevel% neq 0 (" << std::endl;
        ofs << L"    echo Error: command failed with exit code %errorlevel%" << std::endl;
        ofs << L"    exit /b %errorlevel%" << std::endl;
        ofs << L")\n" << std::endl;
    };

    // 临时修改环境变量。将当前程序的目录，以及用户确认的安装目录。但如果使用了setx的话，则不应该将这些临时修改带入进去
    std::function<void()> set_path = [&ofs, this](){
        ofs << L"set TEMP_PATH=%PATH%" << std::endl;
        std::wstring curpath = boost::filesystem::current_path().wstring();
        ofs << L"set PATH=" << curpath << ";%PATH%" << std::endl;
        // 将确认安装的目录临时添加到环境变量中
        for (int t0 = 0; t0 < new_path_list.size(); t0++){
            std::wstring abspath = boost::filesystem::absolute(new_path_list[t0]).wstring();
            ofs << L"set PATH=" << abspath << ";%PATH%" << std::endl;
        }
    };
    set_path();

    // 写入具体命令内容
    for (int t = 0; t < opt_list.size(); t++){
        if (opt_list[t].path.empty()){
            // 执行路径为空字符串，表示在命令行中执行
            // 如果执行命令的内容为setx PATH，则将命令中的%PATH%替换为%TEMP_PATH%，避免环境变量中被加入一些乱七八糟的内容
            std::vector<std::wstring> setx_cmd_list = {L"setx PATH", L"setx \"PATH", L"SETX \"PATH", L"SETX \"PATH"};
            bool is_setx_cmd = false;
            for (auto it = setx_cmd_list.begin(); it != setx_cmd_list.end(); it++){
                if (opt_list[t].command.find(*it) != std::wstring::npos){
                    is_setx_cmd = true;
                    break;
                }
            }
            if (is_setx_cmd){
                std::wstring new_cmd = opt_list[t].command;
                boost::replace_all(new_cmd, L"%PATH%", L"%TEMP_PATH%");
                ofs << new_cmd << std::endl;
                set_path();
            }else{
                ofs << opt_list[t].command << std::endl;
            }
        }else{
            // 执行路径为文件
            std::string filename = "tmp_command/opt";
            filename.append(std::to_string(t));
            filename.append(".txt");
            std::basic_ofstream<wchar_t> ofs_tmp;
            ofs_tmp.open(filename, std::ios::out);
            ofs_tmp << opt_list[t].command;
            ofs_tmp.close();
            // 判断文件所在的文件夹是否存在
            boost::filesystem::path abspath2 = boost::filesystem::absolute(opt_list[t].path).parent_path();
            if (!boost::filesystem::is_directory(abspath2)){
                ofs << L"mkdir " << abspath2.wstring() << std::endl;
                write_check();
            }
            if (opt_list[t].overwrite){
                ofs << L"type \"" << UTF8ToUnicode(filename) << L"\" > \"" << opt_list[t].path << "\"" << std::endl;
            }else{
                ofs << L"type \"" << UTF8ToUnicode(filename) << L"\" >> \"" << opt_list[t].path << "\"" << std::endl;
            }
        }
        // 写入判断执行是否成功的内容
        write_check();
    }
    // 执行命令
    ofs.close();
    proc.start(QString::fromStdWString(L"cmd /k tmp_command\\main.bat"));
}

void CmdExec::on_new_path(std::wstring path)
{
    new_path_list.push_back(path);
}

void CmdExec::reset()
{
    new_path_list.clear();
}
