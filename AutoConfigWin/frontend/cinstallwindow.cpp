#pragma execution_character_set("utf-8")

#include "cinstallwindow.h"
#include "ui_cinstallwindow.h"
#include <QScrollBar>
#include <QFileDialog>
#include <QMessageBox>
#include <iostream>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

void CInstallListItem::init(std::string bkgd_color, std::wstring package_name2)
{
    package_name = package_name2;
    // 设置背景颜色
    QPalette pal(this->palette());
    pal.setColor(QPalette::Base, QColor(bkgd_color.c_str()));
    setAutoFillBackground(true);
    setPalette(pal);

    // 初始化标签
    title = std::make_unique<QLabel>(this);
    button_installed = std::make_unique<QRadioButton>(QString::fromStdWString(L"该库已经安装，选择安装路径"), this);
    button_need_install = std::make_unique<QRadioButton>(QString::fromStdWString(L"该库尚未安装，需要安装"), this);
    button_group = std::make_unique<QButtonGroup>(this);
    button_group->addButton(button_installed.get(), 0);
    button_group->addButton(button_need_install.get(), 0);
    dirname = std::make_unique<QLineEdit>(this);
    button_select = std::make_unique<QPushButton>(this);

    // 设置内容
    std::wstring title_str = package_name;
    title_str.append(L"库");
    title->setText(QString::fromStdWString(title_str));
    button_installed->setChecked(true);
    button_select->setText(QString::fromStdWString(L"选择"));

    // 设置大小
    title->setFixedSize(368, 20);
    button_installed->setFixedSize(368, 20);
    button_need_install->setFixedSize(368, 20);
    dirname->setFixedSize(288, 28);
    button_select->setFixedSize(64, 28);

    // 布局
    title->move(16, 16);
    button_installed->move(16, 48);
    dirname->move(16, 76);
    button_select->move(304, 76);
    button_need_install->move(16, 112);

    connect(button_installed.get(), SIGNAL(toggled(bool)), this, SLOT(on_toggle(bool)));
    connect(button_select.get(), SIGNAL(clicked()), this, SLOT(select_dir()));
}

void CInstallListItem::on_toggle(bool is_checked)
{
    if (is_checked){
        button_select->setEnabled(true);
        dirname->setEnabled(true);
    }else{
        button_select->setEnabled(false);
        dirname->setEnabled(false);
    }
}

void CInstallListItem::select_dir()
{
    std::wstring title = L"输入";
    title.append(package_name);
    title.append(L"库的安装路径");
    QString file_path = QFileDialog::getExistingDirectory(this, QString::fromStdWString(title), "./");
    if (!(file_path.isEmpty())){
        dirname->setText(file_path);
    }
}

CInstallListItem::~CInstallListItem()
{
    disconnect(button_installed.get(), SIGNAL(toggled(bool)), this, SLOT(on_toggle(bool)));
    disconnect(button_select.get(), SIGNAL(clicked()), this, SLOT(select_dir()));
}

CInstallList::CInstallList(QListWidget *parent): QListWidget(parent)
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
                        height: 144px;\
                    }");
}

void CInstallList::init(InstallList install_list)
{
    for (int t = 0; t < install_list.size(); t++){
        item_list.push_back(std::make_unique<CInstallListItem>());
        if (t % 2 == 0){
            item_list[t]->init("#f3f3ff", install_list[t]);
        }else{
            item_list[t]->init("#f3fff3", install_list[t]);
        }
        widget_items.push_back(std::make_unique<QListWidgetItem>());
        this->insertItem(0, widget_items[t].get());
        this->setItemWidget(widget_items[t].get(), item_list[t].get()); //将buddy赋给该newItem
        widget_items[t]->setHidden(false);
    }
}

CInstallWindow::CInstallWindow(InstallList install_list2, std::wstring paragraph, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CInstallWindow)
{
    ui->setupUi(this);
    install_list = install_list2;
    init_graphics(install_list2, paragraph);
}

void CInstallWindow::init_graphics(InstallList install_list2, std::wstring paragraph)
{
    ui->RawGPTContent->setReadOnly(true);
    ui->RawGPTContent->setStyleSheet("background-color:#f3f3f3;");
    ui->RawGPTContent->setText(QString::fromStdWString(paragraph));

    c_install_list = std::make_unique<CInstallList>(ui->cinstall_list);
    c_install_list->setFixedSize(384, 384);
    c_install_list->init(install_list2);
    c_install_list->show();

    connect(ui->ConfirmButton, SIGNAL(clicked()), this, SLOT(confirm_install()));
}

void CInstallWindow::confirm_install()
{
    std::vector<ConfirmInstallRes> confirm_res;
    std::vector<std::wstring> input_error_list;
    // 1.检查是否输入了
    for (int pack_it = 0; pack_it < install_list.size(); pack_it++){
        ConfirmInstallRes res;
        res.package_name = install_list[pack_it];
        if (c_install_list->item_list[pack_it]->button_installed->isChecked()){
            std::wstring install_path = c_install_list->item_list[pack_it]->dirname->text().toStdWString();
            if (install_path.empty()){
                input_error_list.push_back(install_list[pack_it]);
            }
            boost::filesystem::path dirname(install_path);
            if (!boost::filesystem::is_directory(dirname)){
                input_error_list.push_back(install_list[pack_it]);
            }
            res.path = install_path;
        }else{
            res.path = boost::none;
        }
        confirm_res.push_back(res);
    }
    if (!input_error_list.empty()){
        std::wstring err_msg;
        for (int t = 0; t < input_error_list.size(); t++){
            err_msg.append(input_error_list[t]);
            if (t != input_error_list.size() - 1){
                err_msg.append(L"、");
            }
        }
        err_msg.append(L"的安装确认结果输入有误");
        QMessageBox::critical(this, QString::fromStdWString(L"错误信息"), QString::fromStdWString(err_msg));
        return;
    }
    // 2.检查安装路径下是否存在对应的库
    std::vector<std::pair<std::wstring, std::wstring>> input_warn_list;
    for (int t = 0; t < confirm_res.size(); t++){
        if (confirm_res[t].path.has_value()){
            boost::filesystem::path dirname(confirm_res[t].path.value());
            bool has_package = false;
            if (boost::filesystem::is_directory(dirname)){
                for (const auto& it: boost::filesystem::directory_iterator(dirname)){
                    std::wstring filename = it.path().filename().wstring();
                    std::wstring prefix(confirm_res[t].package_name);
                    prefix.push_back(L'.');
                    if ((!boost::filesystem::is_directory(filename)) && boost::istarts_with(filename, prefix)){
                        has_package = true;
                        break;
                    }
                }
            }
            if (!has_package){
                input_warn_list.push_back(std::make_pair(confirm_res[t].package_name, confirm_res[t].path.value()));
            }
        }
    }
    if (input_warn_list.size() >= 1){
        std::wstring msg;
        for (int t = 0; t < input_warn_list.size(); t++){
            msg.append(input_warn_list[t].first);
            msg.append(L" 库的输入目录 ");
            msg.append(input_warn_list[t].second);
            msg.append(L" 下没有对应文件名的文件\n\n");
        }
        msg.append(L"这些目录的输入是否确认正确？");

        QMessageBox msgbox(this);
        msgbox.setIcon(QMessageBox::Question);
        msgbox.setWindowTitle(QString::fromStdWString(L"确认输入"));
        msgbox.setText(QString::fromStdWString(msg));
        msgbox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgbox.setDefaultButton(QMessageBox::No);
        msgbox.button(QMessageBox::Yes)->setText(QString::fromStdWString(L"确认无误"));
        msgbox.button(QMessageBox::No)->setText(QString::fromStdWString(L"返回"));
        if (msgbox.exec() == QMessageBox::No){
            return;
        }
    }
    confirm_close = true;
    this->close();
    emit CommitConfirmInstallRes(confirm_res);
}

void CInstallWindow::closeEvent(QCloseEvent* e)
{
    if (confirm_close){
        e->accept();
        return;
    }
    int exec_result;
    {
        QMessageBox msgbox(this);
        msgbox.setIcon(QMessageBox::Warning);
        msgbox.setWindowTitle(QString::fromStdWString(L"确认退出"));
        msgbox.setText(QString::fromStdWString(L"安装路径尚未确认，如退出将被视为放弃。是否确认退出？"));
        msgbox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgbox.setDefaultButton(QMessageBox::No);
        msgbox.button(QMessageBox::Yes)->setText(QString::fromStdWString(L"放弃并退出"));
        msgbox.button(QMessageBox::No)->setText(QString::fromStdWString(L"返回"));
        exec_result = msgbox.exec();
    }

    if (exec_result == QMessageBox::No){
        e->ignore();
    }else{
        e->accept();
        emit GiveUp();
    }
}

CInstallWindow::~CInstallWindow()
{
    disconnect(ui->ConfirmButton, SIGNAL(clicked()), this, SLOT(confirm_install()));
    delete ui;
}
