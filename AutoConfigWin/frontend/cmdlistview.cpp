#pragma execution_character_set("utf-8")

#include "cmdlistview.h"
#include <QMessageBox>
#include <boost/filesystem.hpp>
#include <QDebug>

void CmdListItem::init(std::string bkgd_color, RealOpt opt, int idx2)
{
    idx = idx2;
    // 设置背景颜色
    QPalette pal(this->palette());
    pal.setColor(QPalette::Base, QColor(bkgd_color.c_str()));
    setAutoFillBackground(true);
    setPalette(pal);

    // 初始化标签
    title = std::make_unique<QLabel>(this);
    path_msg = std::make_unique<QLabel>(QString::fromStdWString(L"执行位置"), this);
    path_text = std::make_unique<QLineEdit>(this);
    content_msg = std::make_unique<QLabel>(QString::fromStdWString(L"执行内容"), this);
    content_text = std::make_unique<QTextEdit>(this);
    button_overwrite = std::make_unique<QRadioButton>(QString::fromStdWString(L"覆写该文件"), this);
    button_append = std::make_unique<QRadioButton>(QString::fromStdWString(L"追加到末尾"), this);
    button_group = std::make_unique<QButtonGroup>(this);
    button_group->addButton(button_overwrite.get(), 0);
    button_group->addButton(button_append.get(), 0);
    button_edit = std::make_unique<QPushButton>(QString::fromStdWString(L"编辑"), this);
    button_del = std::make_unique<QPushButton>(QString::fromStdWString(L"删除"), this);
    button_up = std::make_unique<QPushButton>(QString::fromStdWString(L"上移"), this);
    button_down = std::make_unique<QPushButton>(QString::fromStdWString(L"下移"), this);
    button_save = std::make_unique<QPushButton>(QString::fromStdWString(L"保存"), this);
    button_nosave = std::make_unique<QPushButton>(QString::fromStdWString(L"不保存"), this);

    // 设置内容
    std::wstring title_str = std::to_wstring(idx + 1);
    title_str.append(L". ");
    if (opt.modified == true){
        title_str.append(L"自定义");
    }else{
    switch (opt.opt.opt_type){
        case Download:
            title_str.append(L"下载");
            break;
        case Install:
            title_str.append(L"安装");
            break;
        case Exec:
            title_str.append(L"执行命令");
            break;
        default:
            title_str.append(L"未知，可能有误");
            break;
        }
    }
    title->setText(QString::fromStdWString(title_str));
    path_text->setText(QString::fromStdWString(opt.opt.file_path));
    content_text->setText(QString::fromStdWString(opt.opt.command));
    button_save->hide();
    button_nosave->hide();
    path_text->setReadOnly(true);
    path_text->setStyleSheet("background-color:#f3f3f3;");
    path_text->setPlaceholderText(QString::fromStdWString(L"留空表示在命令行中执行"));
    content_text->setReadOnly(true);
    content_text->setStyleSheet("background-color:#f3f3f3;");
    if (idx == 0){
        button_up->setEnabled(false);
    }
    if (idx == pview->command_num - 1){
        button_down->setEnabled(false);
    }

    // 设置大小
    title->setFixedSize(416, 20);
    path_msg->setFixedSize(80, 20);
    path_text->setFixedSize(304, 28);
    content_msg->setFixedSize(384, 20);
    content_text->setFixedSize(384, 76);
    button_overwrite->setFixedSize(144, 20);
    button_append->setFixedSize(144, 20);
    button_edit->setFixedSize(96, 28);
    button_del->setFixedSize(64, 28);
    button_up->setFixedSize(64, 28);
    button_down->setFixedSize(64, 28);
    button_save->setFixedSize(64, 28);
    button_nosave->setFixedSize(64, 28);

    // 布局
    title->move(16, 16);
    path_msg->move(16, 48);
    path_text->move(96, 44);
    button_overwrite->move(16, 80);
    button_append->move(232, 80);
    content_msg->move(16, 112);
    content_text->move(16, 140);
    button_edit->move(32, 224);
    button_del->move(168, 224);
    button_up->move(248, 224);
    button_down->move(328, 224);
    button_save->move(12, 224);
    button_nosave->move(84, 224);

    connect(button_edit.get(), SIGNAL(clicked()), this, SLOT(on_edit()));
    connect(button_save.get(), SIGNAL(clicked()), this, SLOT(on_save()));
    connect(button_nosave.get(), SIGNAL(clicked()), this, SLOT(on_nosave()));
    connect(button_up.get(), SIGNAL(clicked()), this, SLOT(on_up()));
    connect(button_down.get(), SIGNAL(clicked()), this, SLOT(on_down()));
    connect(button_del.get(), SIGNAL(clicked()), this, SLOT(on_del()));
    connect(button_overwrite.get(), SIGNAL(toggled(bool)), this, SLOT(on_choose_overwrite(bool)));
    connect(button_append.get(), SIGNAL(toggled(bool)), this, SLOT(on_choose_append(bool)));
}

void CmdListItem::on_choose_overwrite(bool is_checked)
{
    if (is_checked){
        pview->real_opt_list[idx].overwrite_status = Overwrite;
    }
}

void CmdListItem::on_choose_append(bool is_checked)
{
    if (is_checked){
        pview->real_opt_list[idx].overwrite_status = Append;
    }
}

void CmdListItem::on_edit()
{
    button_edit->hide();
    button_save->show();
    button_nosave->show();
    path_text->setReadOnly(false);
    path_text->setStyleSheet("background-color:#ffffff;");
    content_text->setReadOnly(false);
    content_text->setStyleSheet("background-color:#ffffff;");
}

void CmdListItem::on_save()
{
    button_edit->show();
    button_save->hide();
    button_nosave->hide();
    path_text->setReadOnly(true);
    path_text->setStyleSheet("background-color:#f3f3f3;");
    content_text->setReadOnly(true);
    content_text->setStyleSheet("background-color:#f3f3f3;");

    pview->real_opt_list[idx].opt.file_path = path_text->text().toStdWString();
    pview->real_opt_list[idx].opt.command = content_text->toPlainText().toStdWString();
    pview->real_opt_list[idx].modified = true;
    std::wstring title_str = std::to_wstring(idx + 1);
    title_str.append(L". 自定义");
    title->setText(QString::fromStdWString(title_str));

    qDebug() << "修改了第" << idx << "号命令。";
}

void CmdListItem::on_nosave()
{
    button_edit->show();
    button_save->hide();
    button_nosave->hide();
    path_text->setReadOnly(true);
    path_text->setStyleSheet("background-color:#f3f3f3;");
    content_text->setReadOnly(true);
    content_text->setStyleSheet("background-color:#f3f3f3;");

    path_text->setText(QString::fromStdWString(pview->real_opt_list[idx].opt.file_path));
    content_text->setText(QString::fromStdWString(pview->real_opt_list[idx].opt.command));
}

void CmdListItem::on_up()
{
    if (idx == 0){
        return;
    }
    pview->on_swap(idx - 1, idx);
}

void CmdListItem::on_down()
{
    if (idx == pview->command_num - 1){
        return;
    }
    pview->on_swap(idx, idx + 1);
}

void CmdListItem::on_del()
{
    int exec_result;
    {
        QMessageBox msgbox(this);
        msgbox.setIcon(QMessageBox::Warning);
        msgbox.setWindowTitle(QString::fromStdWString(L"确认删除"));
        msgbox.setText(QString::fromStdWString(L"是否删除这条指令（此操作不能撤销）？"));
        msgbox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgbox.setDefaultButton(QMessageBox::No);
        msgbox.button(QMessageBox::Yes)->setText(QString::fromStdWString(L"删除"));
        msgbox.button(QMessageBox::No)->setText(QString::fromStdWString(L"返回"));
        exec_result = msgbox.exec();
    }
    if (exec_result == QMessageBox::Yes){
        pview->on_del(idx);
    }
}

CmdListItem::~CmdListItem()
{
    disconnect(button_edit.get(), SIGNAL(clicked()), this, SLOT(on_edit()));
    disconnect(button_save.get(), SIGNAL(clicked()), this, SLOT(on_save()));
    disconnect(button_nosave.get(), SIGNAL(clicked()), this, SLOT(on_nosave()));
    disconnect(button_up.get(), SIGNAL(clicked()), this, SLOT(on_up()));
    disconnect(button_down.get(), SIGNAL(clicked()), this, SLOT(on_down()));
    disconnect(button_del.get(), SIGNAL(clicked()), this, SLOT(on_del()));
    disconnect(button_overwrite.get(), SIGNAL(toggled(bool)), this, SLOT(on_choose_overwrite(bool)));
    disconnect(button_append.get(), SIGNAL(toggled(bool)), this, SLOT(on_choose_append(bool)));
}

CmdListView::CmdListView(QListWidget *parent): QListWidget(parent)
{
    setFocusPolicy(Qt::NoFocus); //去除item选中时的虚线边框
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff); //水平滚动条关闭
    setVerticalScrollMode(QListWidget::ScrollPerPixel); //垂直方向允许按像素精细滚动

    //设置列表样式
    this->setStyleSheet("QListWidget{\
                        background:white;\
                        border:none;\
                    }\
                    QListWidget::item{\
                        border:none;\
                        height: 256px;\
                    }");
}

void CmdListView::init(std::vector<Opt> opt_list)
{
    command_num = opt_list.size();
    for (int t = 0; t < opt_list.size(); t++){

        real_opt_list.push_back(RealOpt{opt_list[t], NotChosen, false});
        CmdListItem* new_item = new CmdListItem(this);
        if (t % 2 == 0){
            new_item->init("#fcfffc", RealOpt{opt_list[t], NotChosen, false}, t);
        }else{
            new_item->init("#f0fff0", RealOpt{opt_list[t], NotChosen, false}, t);
        }
        QListWidgetItem* new_widget_item = new QListWidgetItem(this);
        this->addItem(new_widget_item);
        this->setItemWidget(new_widget_item, new_item); //将buddy赋给该newItem
        new_widget_item->setHidden(false);
    }
}

void CmdListView::rerander()
{
    this->clear();
    for (int t = 0; t < real_opt_list.size(); t++){
        CmdListItem* new_item = new CmdListItem(this);
        if (t % 2 == 0){
            new_item->init("#fcfffc", real_opt_list[t], t);
        }else{
            new_item->init("#f0fff0", real_opt_list[t], t);
        }
        QListWidgetItem* new_widget_item = new QListWidgetItem(this);
        this->addItem(new_widget_item);
        this->setItemWidget(new_widget_item, new_item); //将buddy赋给该newItem
        new_widget_item->setHidden(false);
    }
}

void CmdListView::on_swap(int dx1, int dx2)
{
    qDebug() << "第" << dx1 << "和第" << dx2 << "号命令互换了位置。";
    // 交换这两个命令
    RealOpt tmp_opt = real_opt_list[dx1];
    real_opt_list[dx1] = real_opt_list[dx2];
    real_opt_list[dx2] = tmp_opt;
    // 重新渲染
    rerander();
    this->setCurrentRow(dx1);
}

void CmdListView::on_del(int dx)
{
    qDebug() << "删除了第" << dx << "号命令。";
    // 删除这个指令
    int dx2 = 0;
    for (auto iter = real_opt_list.begin(); iter != real_opt_list.end(); iter++){
        if (dx2 == dx){
            real_opt_list.erase(iter);
            break;
        }
        dx2 += 1;
    }
    // 重新渲染
    rerander();
}

void CmdListView::add_newcmd()
{
    qDebug() << "在末尾处增加了一条命令。";
    // 生成一个新的命令
    RealOpt tmpopt;
    tmpopt.opt = Opt{Exec, L"", L""};
    tmpopt.overwrite_status = NotChosen;
    tmpopt.modified = true;
    real_opt_list.push_back(tmpopt);
    // 重新渲染
    rerander();
    this->setCurrentRow(real_opt_list.size() - 1);
}

boost::variant<RealCmdList, std::wstring> CmdListView::submit()
{
    // 提交确认后的执行命令列表
    RealCmdList res;
    std::wstring err_msg;
    if (real_opt_list.empty()){
        err_msg = L"找不到要执行的指令\n";
    }
    for (int t = 0; t < real_opt_list.size(); t++){
        bool is_overwrite = (real_opt_list[t].overwrite_status == Overwrite);
        res.push_back(RealCmd{real_opt_list[t].opt.file_path, is_overwrite, real_opt_list[t].opt.command});
        // 检查的“是否覆写”的选择是否正确
        if (!real_opt_list[t].opt.file_path.empty()){
            boost::filesystem::path pth(real_opt_list[t].opt.file_path);
            if (boost::filesystem::exists(pth) && real_opt_list[t].overwrite_status == NotChosen){
                err_msg.append(real_opt_list[t].opt.file_path);
                err_msg.append(L"已经存在，需要选择追加还是覆写的方式打开\n");
            }
        }
    }
    if (err_msg.empty()){
        return res;
    }else{
        return err_msg;
    }
}

void RawGPTWindow::init(std::wstring gpt_content)
{
    this->setFixedSize(464, 528);
    textview = std::make_unique<QTextEdit>(this);
    textview->setFixedSize(432, 496);
    textview->move(16, 16);
    textview->setReadOnly(true);
    textview->setText(QString::fromStdWString(gpt_content));
}
