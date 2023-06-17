#pragma execution_character_set("utf-8")

#include "cpathwindow.h"
#include "ui_cpathwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <boost/filesystem.hpp>

void CPathListItem::init(std::string bkgd_color, std::pair<std::wstring, std::wstring> path_str, int dx)
{
    res_dx = dx;
    // 设置背景颜色
    QPalette pal(this->palette());
    pal.setColor(QPalette::Base, QColor(bkgd_color.c_str()));
    setAutoFillBackground(true);
    setPalette(pal);

    // 初始化标签
    title = std::make_unique<QLabel>(this);
    button_file = std::make_unique<QRadioButton>(QString::fromStdWString(L"该命令在文件中执行，输入文件路径"), this);
    button_cmd = std::make_unique<QRadioButton>(QString::fromStdWString(L"该命令在命令行中执行"), this);
    button_cmd2 = std::make_unique<QRadioButton>(QString::fromStdWString(L"在命令行中cd到某一文件夹下执行"), this);
    button_group = std::make_unique<QButtonGroup>(this);
    button_group->addButton(button_file.get(), 0);
    button_group->addButton(button_cmd.get(), 0);
    button_group->addButton(button_cmd2.get(), 0);
    filename = std::make_unique<QLineEdit>(this);
    button_select = std::make_unique<QPushButton>(this);
    cdname = std::make_unique<QLineEdit>(this);
    button_selectcd = std::make_unique<QPushButton>(this);

    // 设置内容
    std::wstring title_str = path_str.second;
    title_str.append(L"\n\n请问 ");
    if (path_str.first.empty()){
        title_str.append(L"执行路径是？");
    }else{
        title_str.append(path_str.first);
        title_str.append(L" 的路径是？");
    }
    title->setText(QString::fromStdWString(title_str));
    button_file->setChecked(true);
    button_select->setText(QString::fromStdWString(L"选择"));
    button_selectcd->setText(QString::fromStdWString(L"选择"));
    button_selectcd->setEnabled(false);
    cdname->setEnabled(false);

    // 设置大小
    title->setFixedSize(368, 80);
    button_file->setFixedSize(368, 20);
    button_cmd->setFixedSize(368, 20);
    button_cmd2->setFixedSize(368, 20);
    filename->setFixedSize(288, 28);
    button_select->setFixedSize(64, 28);
    cdname->setFixedSize(288, 28);
    button_selectcd->setFixedSize(64, 28);

    // 布局
    title->move(16, 16);
    button_file->move(16, 96);
    filename->move(16, 128);
    button_select->move(304, 128);
    button_cmd->move(16, 160);
    button_cmd2->move(16, 192);
    cdname->move(16, 224);
    button_selectcd->move(304, 224);

    connect(button_file.get(), SIGNAL(toggled(bool)), this, SLOT(on_toggle(bool)));
    connect(button_select.get(), SIGNAL(clicked()), this, SLOT(select_dir()));
    connect(button_cmd2.get(), SIGNAL(toggled(bool)), this, SLOT(on_toggle2(bool)));
    connect(button_selectcd.get(), SIGNAL(clicked()), this, SLOT(select_cd()));
}

void CPathListItem::on_toggle(bool is_checked)
{
    if (is_checked){
        button_select->setEnabled(true);
        filename->setEnabled(true);
    }else{
        button_select->setEnabled(false);
        filename->setEnabled(false);
    }
}

void CPathListItem::on_toggle2(bool is_checked)
{
    if (is_checked){
        button_selectcd->setEnabled(true);
        cdname->setEnabled(true);
    }else{
        button_selectcd->setEnabled(false);
        cdname->setEnabled(false);
    }
}

void CPathListItem::select_dir()
{
    std::wstring title = L"选择文件路径";
    QString file_path = QFileDialog::getSaveFileName(this, QString::fromStdWString(title), "", "*.*");
    if (!(file_path.isEmpty())){
        filename->setText(file_path);
    }
}

void CPathListItem::select_cd()
{
    std::wstring title = L"选择要cd的路径";
    QString file_path = QFileDialog::getExistingDirectory(this, QString::fromStdWString(title), "./");
    if (!(file_path.isEmpty())){
        cdname->setText(file_path);
    }
}

CPathListItem::~CPathListItem()
{
    disconnect(button_file.get(), SIGNAL(toggled(bool)), this, SLOT(on_toggle(bool)));
    disconnect(button_select.get(), SIGNAL(clicked()), this, SLOT(select_dir()));
    disconnect(button_cmd2.get(), SIGNAL(toggled(bool)), this, SLOT(on_toggle2(bool)));
    disconnect(button_selectcd.get(), SIGNAL(clicked()), this, SLOT(select_cd()));
}

CPathList::CPathList(QListWidget *parent): QListWidget(parent)
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
                        height:256px;\
                    }");
}

void CPathList::init(PathList path_list)
{
    for (int t = 0; t < path_list.size(); t++){
        item_list.push_back(std::make_unique<CPathListItem>());
        if (t % 2 == 0){
            item_list[t]->init("#f3f3ff", path_list[t], t);
        }else{
            item_list[t]->init("#f3fff3", path_list[t], t);
        }
        widget_items.push_back(std::make_unique<QListWidgetItem>());
        this->insertItem(0, widget_items[t].get());
        this->setItemWidget(widget_items[t].get(), item_list[t].get()); //将buddy赋给该newItem
        widget_items[t]->setHidden(false);
    }

}

CPathWindow::CPathWindow(PathList path_list2, ReplaceList replace_list2, std::wstring paragraph, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CPathWindow)
{
    ui->setupUi(this);
    path_list = path_list2;
    replace_list = replace_list2;
    raw_para = paragraph;
    init_graphics(path_list2, replace_list2, paragraph);
}

void CPathWindow::init_graphics(PathList path_list2, ReplaceList replace_list2, std::wstring paragraph)
{
    ui->RawGPTContent->setReadOnly(true);
    ui->RawGPTContent->setStyleSheet("background-color:#f3f3f3;");
    ui->RawGPTContent->setText(QString::fromStdWString(paragraph));

    if (path_list2.size() > 0){
        c_path_list = std::make_unique<CPathList>(ui->cpath_list);
        c_path_list->setFixedSize(384, 384);
        c_path_list->init(path_list2);
        c_path_list->show();
    }else{
        ui->cpath_list->hide();
        ui->label_4->setText(QString::fromStdWString(L"无需要确认的执行路径"));
    }

    if (replace_list2.size() > 0){
        c_replace_list = std::make_unique<CReplaceList>(ui->creplace_list);
        c_replace_list->setFixedSize(384, 384);
        c_replace_list->init(replace_list2);
        c_replace_list->show();
    }else{
        ui->creplace_list->hide();
        ui->label_3->setText(QString::fromStdWString(L"无需要确认的替换项"));
    }

    connect(ui->ConfirmButton, SIGNAL(clicked()), this, SLOT(confirm_path()));
}

void CPathWindow::confirm_path()
{
    ConfirmPathRes path_res;
    ConfirmReplaceRes replace_res;
    path_res.names = path_list;
    path_res.values = std::vector<std::wstring>(path_list.size(), L"");
    replace_res.names = replace_list;
    replace_res.values = std::vector<std::wstring>(replace_list.size(), L"");
    std::vector<std::pair<std::wstring, std::wstring>> input_error_list;
    std::wstring text = L"GPT原始返回的内容是：\n\n";
    text.append(raw_para);
    text.append(L"\n\n");
    // 1.生成替换内容的确认结果
    if (replace_list.size() >= 1){
        text.append(L"其中，");
        for (int t = 0; t < replace_list.size(); t++){
            int res_dx = c_replace_list->item_list[t]->res_dx;
            std::wstring value = c_replace_list->item_list[t]->pathname->text().toStdWString();
            if (c_replace_list->item_list[t]->button_need_replace->isChecked()){
                value = c_replace_list->item_list[t]->pathname->text().toStdWString();
                text.append(std::get<0>(replace_res.names[res_dx]));
                text.append(L" 将被替换为 ");
                text.append(value);
            }else{
                value = std::get<0>(replace_res.names[res_dx]);
                text.append(std::get<0>(replace_res.names[res_dx]));
                text.append(L" 将不被替换");
            }
            replace_res.values[res_dx] = value;
            if (t != replace_list.size()){
                text.append(L"，");
            }
        }
        text.append(L"\n\n");
    }
    // 2.检查执行的路径是否正确输入了
    for (int t = 0; t < path_list.size(); t++){
        int res_dx = c_path_list->item_list[t]->res_dx;
        //std::wstring value = c_replace_list->item_list[t]->pathname->text().toStdWString();
        //res.values[res_dx] = value;
        text.append(path_list[res_dx].second);
        text.append(L" 的执行路径为 ");
        if (c_path_list->item_list[t]->button_cmd->isChecked()){
            // 这个命令的执行地点为命令行，无需进一步确认
            path_res.values[res_dx] = L"";
            text.append(L" 命令行\n");
        }else if (c_path_list->item_list[t]->button_cmd2->isChecked()){
            // 这个命令的执行地点为cd到某一个文件夹下，检查cd路径输入是否准确。要求路径名不为空，且必须存在
            std::wstring cdname = c_path_list->item_list[t]->cdname->text().toStdWString();
            if (cdname.empty()){
                input_error_list.push_back(std::make_pair(path_list[res_dx].second, L"未输入要cd的路径"));
                continue;
            }
            boost::filesystem::path filepath(cdname);
            if (!boost::filesystem::is_directory(cdname)){
                input_error_list.push_back(std::make_pair(path_list[res_dx].second, L"要cd的目录不存在"));
                continue;
            }
            path_res.values[res_dx] = L"cd:";
            path_res.values[res_dx].append(cdname);
            text.append(L"命令行中cd到 ");
            text.append(cdname);
            text.append(L"\n");
        }else{
            // 这个命令的执行地点为某一个文件，检查文件路径输入是否准确。要求文件名不为空，且其目录必须存在
            std::wstring filename = c_path_list->item_list[t]->filename->text().toStdWString();
            if (filename.empty()){
                input_error_list.push_back(std::make_pair(path_list[res_dx].second, L"未输入文件路径"));
                continue;
            }
            boost::filesystem::path filepath(filename);
            boost::filesystem::path dirname(filepath.parent_path());
            if (!boost::filesystem::is_directory(dirname)){
                input_error_list.push_back(std::make_pair(path_list[res_dx].second, L"文件路径所在目录不存在"));
                continue;
            }
            path_res.values[res_dx] = filename;
            text.append(filename);
            text.append(L"\n");
        }
    }
    text.append(L"\n是否确认？");
    if (!input_error_list.empty()){
        std::wstring err_msg = L"部分命令的执行路径输入有误\n\n";
        for (int t = 0; t < input_error_list.size(); t++){
            err_msg.append(input_error_list[t].first);
            err_msg.append(L"：");
            err_msg.append(input_error_list[t].second);
            if (t != input_error_list.size() - 1){
                err_msg.append(L"\n");
            }
        }
        QMessageBox::critical(this, QString::fromStdWString(L"错误信息"), QString::fromStdWString(err_msg));
        return;
    }
    // 3.弹出确认框，如果确认无误则直接提交
    {
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
    }
    confirm_close = true;
    this->close();
    emit CommitConfirmPathRes(path_res, replace_res);
}

void CPathWindow::closeEvent(QCloseEvent* e)
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
        msgbox.setText(QString::fromStdWString(L"命令执行内容和方式尚未确认，如退出将被视为放弃。是否确认退出？"));
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

CPathWindow::~CPathWindow()
{
    disconnect(ui->ConfirmButton, SIGNAL(clicked()), this, SLOT(confirm_path()));
    delete ui;
}
