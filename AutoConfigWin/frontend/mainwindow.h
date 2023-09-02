#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "question.h"
#include "gpt_comm.h"
#include "advopdialog.h"
#include "cinstallwindow.h"
#include "creplacewindow.h"
#include "cpathwindow.h"
#include "cmdlistview.h"
#include "../cmdexec.h"
#include <QMainWindow>
#include <QStandardItemModel>
#include <boost/optional.hpp>
#include <QDialog>
#include <QMessageBox>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(GptComm* gpt_comm_, CmdExec* cmd_exec_, QWidget *parent = nullptr);
    ~MainWindow();

    void init_graphics(); //初始化主界面
    bool eventFilter(QObject *obj, QEvent *event);
    void textedit_log_handler(QtMsgType type, const QMessageLogContext &context, const QString &msg); //将日志重定向至textedit中

    enum Status
    {
        Init = 0,
        Downloading,
        Parsing,
        PrepareExecute,
        Execute
    };

signals:
    void append_log(QString log); //向日志界面中添加日志

public slots:
    void on_add_env_line(); //点击按钮，需要在“环境配置”中增加一行时，执行这个事件
    void on_show_adv_op(); //点击按钮显示高级选项
    void on_confirm_adv_op(); //确认按钮显示的高级选项
    void on_giveup_adv_op(); //放弃按钮显示的高级选项
    void submit_install_question(); //点击“确定”按钮后，提交
    void submit_config_question(); //点击“确定”按钮后，提交
    void show_raw_gptanswer();  //展示原始返回的内容
    void on_answer(std::wstring origin_answer, std::wstring check_answer); //从GPT获取到问题答案的返回内容
    void on_error_msg(int error_id); //从GPT获取答案的过程中发生错误的返回内容
    void exec();

    // AnswerParser与确认界面相关的信号
    void on_need_next_question(std::wstring question);
    void on_need_confirm_install(InstallList package_name, std::wstring paragraph);
    void on_confirm_install(std::vector<ConfirmInstallRes> res);
    void on_need_confirm_replace(ReplaceList replace_question, std::wstring paragraph);
    void on_confirm_replace(ConfirmReplaceRes res);
    void on_need_confirm_path(PathList path_question, ReplaceList replace_question, std::wstring paragraph);
    void on_confirm_path(ConfirmPathRes path_res, ConfirmReplaceRes replace_res);
    void on_confirm_give_up();
    void on_parse_success(std::vector<Opt> opt_list);

    // 命令执行完成和失败的回调
    void on_exec_success(std::wstring rtn_msg);
    void on_exec_failed(std::wstring err_msg);

    // 添加日志的回调
    void on_append_log(QString log);
private:
    Ui::MainWindow *ui;

    QStandardItemModel* env_model;
    GptComm* gpt_comm;
    CmdExec* cmd_exec;
    std::unique_ptr<AnswerParser> parse_answer = nullptr; // 问题解析环节
    AdvOpDialog adv_op_dialog; // 展示高级选项
    std::unique_ptr<CInstallWindow> cinstall_window = nullptr; // 确认安装路径的界面
    std::unique_ptr<CReplaceWindow> creplace_window = nullptr; // 确认替换内容的界面
    std::unique_ptr<CPathWindow> cpath_window = nullptr; // 确认命令执行的路径和替换内容的界面
    std::unique_ptr<RawGPTWindow> gpt_window = nullptr; // 展示原始GPT返回内容的界面
    std::unique_ptr<CmdListView> cmd_list_view = nullptr; //展示从GPT解析完成之后的命令列表

    int venv_option = VenvForbid; //是否启用容器、虚拟环境的选项
    int num_env_line = 0;
    std::wstring input_os; //询问问题的操作系统
    boost::optional<std::wstring> package_to_install; //询问需要安装的软件名
    std::wstring raw_answer_ws; //GPT返回的原始的答案字符串
    int status = Init; //这个模块的状态：初始/下载中/解析中/解析成功/执行中
    void connect_parser_signals(); //将AnswerParser中的一些信号/插槽，与GptComm、子窗口中的一些信号/插槽相连接
    void show_report_page(); //展示“反馈改进意见和bug”的页面
    QDialog* wait_dialog; //显示“请稍后”的对话框

//    QMessageBox* wait_msgbox;
};

#endif // MAINWINDOW_H
