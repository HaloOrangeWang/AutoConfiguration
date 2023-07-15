#pragma execution_character_set("utf-8")

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "funcs.h"
#include <QMessageBox>
#include <QDebug>
#include <QHBoxLayout>
#include <QDesktopServices>

MainWindow::MainWindow(GptComm* gpt_comm_, CmdExec* cmd_exec_, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    gpt_comm = gpt_comm_;
    cmd_exec = cmd_exec_;
    ui->setupUi(this);
    init_graphics();
}

void MainWindow::init_graphics()
{
    //初始化“软件环境”的表格。
    env_model = new QStandardItemModel();
    env_model->setHorizontalHeaderItem(0, new QStandardItem(QObject::tr("软件名")));
    env_model->setHorizontalHeaderItem(1, new QStandardItem(QObject::tr("路径")));
    ui->tableView_Env->setModel(env_model);

    // 初始化展示从GPT解析出来的结果的界面
    cmd_list_view = std::make_unique<CmdListView>(ui->copt_list);
    cmd_list_view->setFixedSize(432, 496);
    ui->button_addcmd->setEnabled(false);
    ui->button_cmdexec->setEnabled(false);

    ui->label_rawgpt->setStyleSheet("QLabel{\
                                     color:#999999;\
                                     }");
    ui->label_rawgpt->installEventFilter(this);
    ui->label_report->setStyleSheet("QLabel{\
                                     color:#4444bb;\
                                     }");
    ui->label_report->installEventFilter(this);
    ui->text_log->setReadOnly(true);
    ui->text_log->setStyleSheet("background-color:#f3f3f3;");

    // 初始化“请稍后”的对话框，但不展示
    wait_dialog = new QDialog(this);
    wait_dialog->setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    wait_dialog->setWindowTitle("请稍后");
    wait_dialog->setFixedSize(384, 144);
    QLabel* label = new QLabel(wait_dialog);
    label->setText("请稍后");
    label->setPixmap(QApplication::style()->standardIcon(QStyle::SP_MessageBoxInformation).pixmap(48, 48));
    label->move(16, 48);
    QLabel* label2 = new QLabel(wait_dialog);
    label2->setText("等待GPT返回内容中...");
    QFont font("楷体", 14);
    label2->setFont(font);
    label2->move(80, 56);
    wait_dialog->hide();

    // 绑定按钮点击事件
    connect(ui->pushButton_AddEnvLine, SIGNAL(clicked()), this, SLOT(on_add_env_line()));
    connect(ui->pushButton_Install, SIGNAL(clicked()), this, SLOT(submit_install_question()));
    connect(ui->pushButton_Conf, SIGNAL(clicked()), this, SLOT(submit_config_question()));
    connect(ui->button_addcmd, SIGNAL(clicked()), cmd_list_view.get(), SLOT(add_newcmd()));
    connect(ui->button_cmdexec, SIGNAL(clicked()), this, SLOT(exec()));

    // 绑定从GPT接收消息的事件
    connect(gpt_comm, SIGNAL(ReturnAnswer(std::wstring)), this, SLOT(on_answer(std::wstring)));
    connect(gpt_comm, SIGNAL(ReturnErrorMsg(int)), this, SLOT(on_error_msg(int)));

    // 绑定命令行执行结果的事件
    connect(cmd_exec, SIGNAL(on_exec_success(std::wstring)), this, SLOT(on_exec_success(std::wstring)));
    connect(cmd_exec, SIGNAL(on_exec_failed(std::wstring)), this, SLOT(on_exec_failed(std::wstring)));

    // 绑定日志处理的事件
    connect(this, SIGNAL(append_log(QString)), this, SLOT(on_append_log(QString)));
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == ui->label_rawgpt){
        if (event->type() == QEvent::MouseButtonRelease){
            show_raw_gptanswer(); //调用点击事件
        }
    }
    if (obj == ui->label_report){
        if (event->type() == QEvent::MouseButtonRelease){
            show_report_page(); //调用点击事件
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::textedit_log_handler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    emit append_log(msg);
}

void MainWindow::on_add_env_line()
{
    env_model->setItem(num_env_line, 0, new QStandardItem(""));
    env_model->setItem(num_env_line, 1, new QStandardItem(""));
    ui->tableView_Env->verticalHeader()->resizeSection(num_env_line, 24);
    num_env_line += 1;
}

void MainWindow::submit_install_question()
{
    std::wstring env_os = ui->lineEdit_OS->text().toStdWString();
    if (env_os.empty()){
        QMessageBox::about(this, "Hint", QString::fromStdWString(L"需要输入操作系统"));
        return;
    }
    std::vector<std::pair<std::wstring, std::wstring>> env_sw;
    for (int line = 0; line < env_model->rowCount(); line++){
        std::wstring text1 = env_model->item(line, 0)->text().toStdWString();
        std::wstring text2 = env_model->item(line, 1)->text().toStdWString();
        if (!text1.empty()){
            env_sw.push_back(make_pair(text1, text2));
        }
    }
    std::wstring software_name = ui->lineEdit_Install->text().toStdWString();
    if (software_name.empty()){
        QMessageBox::about(this, "Hint", QString::fromStdWString(L"需要输入要安装的软件名"));
        return;
    }
    question = new QuestionInstall(env_os, env_sw, software_name);
    std::wstring question_str = question->format_question();
    input_os = env_os;
    package_to_install = software_name;
    status = Downloading;
    wait_dialog->show();
    gpt_comm->input_question(question_str);
    ui->pushButton_Conf->setEnabled(false);
    ui->pushButton_Install->setEnabled(false);
}

void MainWindow::submit_config_question()
{
    std::wstring env_os = ui->lineEdit_OS->text().toStdWString();
    if (env_os.empty()){
        QMessageBox::about(this, "Hint", QString::fromStdWString(L"需要输入操作系统"));
        return;
    }
    std::vector<std::pair<std::wstring, std::wstring>> env_sw;
    for (int line = 0; line < env_model->rowCount(); line++){
        std::wstring text1 = env_model->item(line, 0)->text().toStdWString();
        std::wstring text2 = env_model->item(line, 1)->text().toStdWString();
        if (!text1.empty()){
            env_sw.push_back(make_pair(text1, text2));
        }
    }
    std::wstring software_name = ui->lineEdit_CfName->text().toStdWString();
    std::wstring software_path = ui->lineEdit_CfPth->text().toStdWString();
    std::wstring description = ui->lineEdit_CfDscp->text().toStdWString();
    if (software_name.empty() || description.empty()){
        QMessageBox::about(this, "Hint", QString::fromStdWString(L"需要输入要配置的软件名及配置方法"));
        return;
    }
    question = new QuestionConfigure(env_os, env_sw, software_name, software_path, description);
    std::wstring question_str = question->format_question();
    input_os = env_os;
    package_to_install = boost::none;
    status = Downloading;
    wait_dialog->show();
    gpt_comm->input_question(question_str);
    ui->pushButton_Conf->setEnabled(false);
    ui->pushButton_Install->setEnabled(false);
}

void MainWindow::show_raw_gptanswer()
{
    if (status <= Parsing){
        return;
    }
    gpt_window = std::make_unique<RawGPTWindow>();
    gpt_window->init(raw_answer_ws);
    gpt_window->show();
}

void MainWindow::connect_parser_signals()
{
    // TODO: GPT解析失败的处理
    connect(parse_answer.get(), SIGNAL(EmitGPTQuestion(std::wstring)), this, SLOT(on_need_next_question(std::wstring)));
    connect(parse_answer.get(), SIGNAL(ConfirmInstall(InstallList, std::wstring)), this, SLOT(on_need_confirm_install(InstallList, std::wstring)));
    connect(parse_answer.get(), SIGNAL(ConfirmReplace(ReplaceList, std::wstring)), this, SLOT(on_need_confirm_replace(ReplaceList, std::wstring)));
    connect(parse_answer.get(), SIGNAL(ConfirmPath(PathList, ReplaceList, std::wstring)), this, SLOT(on_need_confirm_path(PathList, ReplaceList, std::wstring)));
    connect(parse_answer.get(), SIGNAL(ParseSuccess(std::vector<Opt>, std::wstring)), this, SLOT(on_parse_success(std::vector<Opt>, std::wstring)));
}

void MainWindow::on_need_next_question(std::wstring question)
{
    // 需要提出下一个问题
    gpt_comm->send_next_question(question);
}

void MainWindow::on_need_confirm_install(InstallList package_name, std::wstring paragraph)
{
    cinstall_window = std::make_unique<CInstallWindow>(package_name, paragraph);
    connect(cinstall_window.get(), SIGNAL(CommitConfirmInstallRes(std::vector<ConfirmInstallRes>)), this, SLOT(on_confirm_install(std::vector<ConfirmInstallRes>)));
    connect(cinstall_window.get(), SIGNAL(GiveUp()), this, SLOT(on_confirm_give_up()));
    cinstall_window->show();
}

void MainWindow::on_need_confirm_replace(ReplaceList replace_question, std::wstring paragraph)
{
    creplace_window = std::make_unique<CReplaceWindow>(replace_question, paragraph);
    connect(creplace_window.get(), SIGNAL(CommitConfirmReplaceRes(ConfirmReplaceRes)), this, SLOT(on_confirm_replace(ConfirmReplaceRes)));
    connect(creplace_window.get(), SIGNAL(GiveUp()), this, SLOT(on_confirm_give_up()));
    creplace_window->show();
}

void MainWindow::on_need_confirm_path(PathList path_question, ReplaceList replace_question, std::wstring paragraph)
{
    cpath_window = std::make_unique<CPathWindow>(path_question, replace_question, paragraph);
    connect(cpath_window.get(), SIGNAL(CommitConfirmPathRes(ConfirmPathRes, ConfirmReplaceRes)), this, SLOT(on_confirm_path(ConfirmPathRes, ConfirmReplaceRes)));
    connect(cpath_window.get(), SIGNAL(GiveUp()), this, SLOT(on_confirm_give_up()));
    cpath_window->show();
}

void MainWindow::on_confirm_give_up()
{
    // TODO: 主动放弃确认的处理
}

void MainWindow::on_confirm_install(std::vector<ConfirmInstallRes> res)
{
    // 完成了输入安装路径的操作后的处理
    disconnect(cinstall_window.get(), SIGNAL(CommitConfirmInstallRes(std::vector<ConfirmInstallRes>)), this, SLOT(on_confirm_install(std::vector<ConfirmInstallRes>)));
    disconnect(cinstall_window.get(), SIGNAL(GiveUp()), this, SLOT(on_confirm_give_up()));
    cinstall_window.reset(nullptr);

    // 将新增的路径保存下来，日后添加到环境变量中
    for (int t = 0; t < res.size(); t++){
        if (res[t].path.has_value()){
            cmd_exec->on_new_path(res[t].path.value());
        }
    }

    parse_answer->on_confirm_install(res);
}

void MainWindow::on_confirm_replace(ConfirmReplaceRes res)
{
    // 完成了输入替换内容的操作后的处理
    disconnect(creplace_window.get(), SIGNAL(CommitConfirmReplaceRes(ConfirmReplaceRes)), this, SLOT(on_confirm_replace(ConfirmReplaceRes)));
    disconnect(creplace_window.get(), SIGNAL(GiveUp()), this, SLOT(on_confirm_give_up()));
    creplace_window.reset(nullptr);

    parse_answer->on_confirm_replace(res);
}

void MainWindow::on_confirm_path(ConfirmPathRes path_res, ConfirmReplaceRes replace_res)
{
    // 完成了输入执行地址和替换内容的操作后的处理
    disconnect(cpath_window.get(), SIGNAL(CommitConfirmPathRes(ConfirmPathRes, ConfirmReplaceRes)), this, SLOT(on_confirm_path(ConfirmPathRes, ConfirmReplaceRes)));
    disconnect(cpath_window.get(), SIGNAL(GiveUp()), this, SLOT(on_confirm_give_up()));
    cpath_window.reset(nullptr);

    parse_answer->on_confirm_path(path_res, replace_res);
}

void MainWindow::on_parse_success(std::vector<Opt> opt_list, std::wstring raw_answer)
{
    status = PrepareExecute;
    //解析GPT返回结果全部成功后的调用内容
    //将解析结果展示在界面上
    raw_answer_ws = raw_answer;
    cmd_list_view->init(opt_list);
    cmd_list_view->show();
    //在界面上展示“未解析出命令”的段落列表
    if (!parse_answer->unmanageable_para_list.empty()){
        std::wstring unmanageable_log = L"以下段落未找到对应的命令：\n\n";
        for (int para_it = 0; para_it < parse_answer->unmanageable_para_list.size(); para_it++){
            unmanageable_log.append(parse_answer->unmanageable_para_list[para_it]);
            unmanageable_log.append(L"\n\n");
        }
        ui->text_log->setText(QString::fromStdWString(unmanageable_log));
    }
    //调整部分按钮的样式
    ui->button_addcmd->setEnabled(true);
    ui->label_rawgpt->setStyleSheet("QLabel{\
                                     color:#4444bb;\
                                     }\
                                     QLabel:hover{\
                                     color:#6666bb;\
                                     }");
    ui->button_cmdexec->setEnabled(true);
}

void MainWindow::exec()
{
    // 确认指令无误，开始执行的内容
    // 1.先获取要执行的内容，以及错误码。
    boost::variant<RealCmdList, std::wstring> res = cmd_list_view->submit();
    if (res.type() == typeid(std::wstring)){
        // 获取要执行的内容未成功
        std::wstring err_msg = boost::get<std::wstring>(res);
        QMessageBox::critical(this, QString::fromStdWString(L"错误信息"), QString::fromStdWString(err_msg));
        return;
    }
    RealCmdList cmd_list = boost::get<RealCmdList>(res);
    // 2.确认无误后可以执行
    std::wstring text;
    for (int t = 0; t < cmd_list.size(); t++){
        text.append(L"在");
        if (cmd_list[t].path.empty()){
            text.append(L"命令行中执行\n");
        }else{
            text.append(L"文件");
            text.append(cmd_list[t].path);
            if (cmd_list[t].overwrite == true){
                text.append(L"中覆盖式写入\n");
            }else{
                text.append(L"末尾处追加\n");
            }
        }
        text.append(cmd_list[t].command);
        text.append(L"\n\n");
    }
    QMessageBox msgbox(this);
    msgbox.setIcon(QMessageBox::Question);
    msgbox.setWindowTitle(QString::fromStdWString(L"确认输入"));
    msgbox.setText(QString::fromStdWString(text));
    msgbox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgbox.setDefaultButton(QMessageBox::No);
    msgbox.button(QMessageBox::Yes)->setText(QString::fromStdWString(L"确认无误"));
    msgbox.button(QMessageBox::No)->setText(QString::fromStdWString(L"返回"));
    if (msgbox.exec() == QMessageBox::No){
        return;
    }
    cmd_exec->exec_real_commands(cmd_list);
    status = Execute;
}

void MainWindow::on_answer(std::wstring message)
{
    qDebug() << "已收到GPT返回的答案";
    if (parse_answer == nullptr){
        // 此次配置的第一个问题：新建一个AnswerParser
        wait_dialog->close();
        parse_answer = std::make_unique<AnswerParser>(input_os, package_to_install, cmd_exec);
        connect_parser_signals();

        //answer_ws.assign(answer.begin(), answer.end());
        status = Parsing;
        parse_answer->parse_first_answer(message);
        raw_answer_ws = message;
    }else{
        // 不是此次配置的第一个问题，使用现有的AnswerParser
        parse_answer->parse_second_answer(message);
    }
}

void MainWindow::on_exec_success(std::wstring rtn_msg)
{
    QMessageBox::about(this, QString::fromStdWString(L"命令执行成功"), QString::fromStdWString(L"命令执行成功"));
    // 状态复位
    status = Init;
    ui->label_rawgpt->setStyleSheet("QLabel{\
                                     color:#999999;\
                                     }");
    ui->button_addcmd->setEnabled(false);
    ui->button_cmdexec->setEnabled(false);
}

void MainWindow::on_exec_failed(std::wstring rtn_msg)
{
    QMessageBox::about(this, QString::fromStdWString(L"命令执行失败"), QString::fromStdWString(rtn_msg));
    // 状态复位
    status = Init;
    ui->label_rawgpt->setStyleSheet("QLabel{\
                                     color:#999999;\
                                     }");
    ui->button_addcmd->setEnabled(false);
    ui->button_cmdexec->setEnabled(false);
}

void MainWindow::on_error_msg(int error_id)
{
    wait_dialog->close();
    if (parse_answer != nullptr){
        parse_answer.reset(nullptr);
    }
    switch (error_id){
    case gpt_comm->Busy:
        qDebug() << "上一个问题还没有处理完，不能继续提问。错误码为" << error_id;
        QMessageBox::about(this, "Hint", QString::fromStdWString(L"上一个问题还没有处理完，不能继续提问"));
        break;
    case gpt_comm->FailedToConnect:
        qDebug() << "与WebSocket服务器连接失败。错误码为" << error_id;
        QMessageBox::about(this, "Hint", QString::fromStdWString(L"与WebSocket服务器连接失败"));
        break;
    case gpt_comm->WebSocketError:
        qDebug() << "与WebSocket服务器连接过程中出现异常。错误码为" << error_id;
        QMessageBox::about(this, "Hint", QString::fromStdWString(L"与WebSocket服务器连接过程中出现异常"));
        break;
    case gpt_comm->ServerError:
        qDebug() << "WebSocket服务器内部异常。错误码为" << error_id;
        QMessageBox::about(this, "Hint", QString::fromStdWString(L"WebSocket服务器内部异常"));
        break;
    case gpt_comm->GPTError:
    default:
        qDebug() << "GPT没有返回任何答案。错误码为" << error_id;
        QMessageBox::about(this, "Hint", QString::fromStdWString(L"GPT没有返回任何答案"));
        break;
    }
}

void MainWindow::on_append_log(QString log)
{
    ui->text_log->append(log);
}

void MainWindow::show_report_page()
{
    QDesktopServices::openUrl(QUrl(ReportURL.c_str()));
}

MainWindow::~MainWindow()
{
    delete ui;
}
