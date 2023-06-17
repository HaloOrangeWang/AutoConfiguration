#pragma execution_character_set("utf-8")

#include "creplacewindow.h"
#include "ui_creplacewindow.h"
#include <QFileDialog>
#include <QMessageBox>

void CReplaceListItem::init(std::string bkgd_color, std::tuple<std::wstring, std::wstring, std::wstring> replace_str, int dx)
{
    res_dx = dx;
    // 设置背景颜色
    QPalette pal(this->palette());
    pal.setColor(QPalette::Base, QColor(bkgd_color.c_str()));
    setAutoFillBackground(true);
    setPalette(pal);

    // 初始化标签
    title = std::make_unique<QLabel>(this);
    button_no_replace = std::make_unique<QRadioButton>(this);
    button_need_replace = std::make_unique<QRadioButton>(this);
    button_group = std::make_unique<QButtonGroup>(this);
    button_group->addButton(button_no_replace.get(), 0);
    button_group->addButton(button_need_replace.get(), 0);
    pathname = std::make_unique<QLineEdit>(this);
    button_select = std::make_unique<QPushButton>(this);
    button_select2 = std::make_unique<QPushButton>(this);

    // 设置内容
    std::wstring title_str = std::get<0>(replace_str);
    title_str.append(L" ");
    title_str.append(std::get<1>(replace_str));
    title_str.append(L" ");
    title_str.append(std::get<2>(replace_str));
    title_str.append(L"\n\n请问是否需要替换 ");
    title_str.append(std::get<0>(replace_str));
    title_str.append(L" ？");
    title->setText(QString::fromStdWString(title_str));
    button_select->setText(QString::fromStdWString(L"选文件"));
    button_select2->setText(QString::fromStdWString(L"选文件夹"));
    std::wstring no_replace_str = L"无需替换";
    no_replace_str.append(std::get<0>(replace_str));
    std::wstring need_replace_str = L"输入 ";
    need_replace_str.append(std::get<2>(replace_str));
    need_replace_str.append(L"的值");
    button_no_replace->setText(QString::fromStdWString(no_replace_str));
    button_need_replace->setText(QString::fromStdWString(need_replace_str));
    button_need_replace->setChecked(true);

    // 设置大小
    title->setFixedSize(368, 96);
    button_no_replace->setFixedSize(368, 20);
    button_need_replace->setFixedSize(368, 20);
    pathname->setFixedSize(288, 28);
    button_select->setFixedSize(64, 20);
    button_select2->setFixedSize(64, 20);

    // 布局
    title->move(16, 16);
    button_need_replace->move(16, 112);
    pathname->move(16, 144);
    button_select->move(304, 134);
    button_select2->move(304, 154);
    button_no_replace->move(16, 176);

    connect(button_select.get(), SIGNAL(clicked()), this, SLOT(select_path()));
    connect(button_select2.get(), SIGNAL(clicked()), this, SLOT(select_path2()));
    connect(button_need_replace.get(), SIGNAL(toggled(bool)), this, SLOT(on_toggle(bool)));
}

void CReplaceListItem::on_toggle(bool is_checked)
{
    if (is_checked){
        button_select->setEnabled(true);
        button_select2->setEnabled(true);
        pathname->setEnabled(true);
    }else{
        button_select->setEnabled(false);
        button_select2->setEnabled(false);
        pathname->setEnabled(false);
    }
}

void CReplaceListItem::select_path()
{
    QString file_path = QFileDialog::getOpenFileName(this, QString::fromStdWString(L"选择路径"), "./", "*.*");
    if (!(file_path.isEmpty())){
        pathname->setText(file_path);
    }
}

void CReplaceListItem::select_path2()
{
    QString file_path = QFileDialog::getExistingDirectory(this, QString::fromStdWString(L"选择路径"), "./");
    if (!(file_path.isEmpty())){
        pathname->setText(file_path);
    }
}

CReplaceListItem::~CReplaceListItem()
{
    disconnect(button_select.get(), SIGNAL(clicked()), this, SLOT(select_path()));
    disconnect(button_select2.get(), SIGNAL(clicked()), this, SLOT(select_path2()));
    disconnect(button_need_replace.get(), SIGNAL(toggled(bool)), this, SLOT(on_toggle(bool)));
}

CReplaceList::CReplaceList(QListWidget *parent): QListWidget(parent)
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
                        height: 208px;\
                    }");
}

void CReplaceList::init(ReplaceList replace_list)
{
    for (int t = 0; t < replace_list.size(); t++){
        item_list.push_back(std::make_unique<CReplaceListItem>());
        if (t % 2 == 0){
            item_list[t]->init("#f3f3ff", replace_list[t], t);
        }else{
            item_list[t]->init("#f3fff3", replace_list[t], t);
        }
        widget_items.push_back(std::make_unique<QListWidgetItem>());
        this->insertItem(0, widget_items[t].get());
        this->setItemWidget(widget_items[t].get(), item_list[t].get()); //将buddy赋给该newItem
        widget_items[t]->setHidden(false);
    }
}

CReplaceWindow::CReplaceWindow(ReplaceList replace_list2, std::wstring paragraph, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CReplaceWindow)
{
    ui->setupUi(this);
    replace_list = replace_list2;
    raw_para = paragraph;
    init_graphics(replace_list2, paragraph);
}

void CReplaceWindow::init_graphics(ReplaceList replace_list2, std::wstring paragraph)
{
    ui->RawGPTContent->setReadOnly(true);
    ui->RawGPTContent->setStyleSheet("background-color:#f3f3f3;");
    ui->RawGPTContent->setText(QString::fromStdWString(paragraph));

    c_replace_list = std::make_unique<CReplaceList>(ui->creplace_list);
    c_replace_list->setFixedSize(384, 384);
    c_replace_list->init(replace_list2);
    c_replace_list->show();

    connect(ui->ConfirmButton, SIGNAL(clicked()), this, SLOT(confirm_replace()));
}

void CReplaceWindow::confirm_replace()
{
    ConfirmReplaceRes res;
    res.names = replace_list;
    res.values = std::vector<std::wstring>(replace_list.size(), L"");
    // 将输入的替换结果展示在QMessageBox中，并交由用户确认。如果确认无误，则将返回
    std::wstring text = L"GPT原始返回的内容是：\n\n";
    text.append(raw_para);
    text.append(L"\n\n其中，");
    for (int t = 0; t < replace_list.size(); t++){
        int res_dx = c_replace_list->item_list[t]->res_dx;
        std::wstring value;
        if (c_replace_list->item_list[t]->button_need_replace->isChecked()){
            value = c_replace_list->item_list[t]->pathname->text().toStdWString();
            text.append(std::get<0>(res.names[res_dx]));
            text.append(L" 将被替换为 ");
            text.append(value);
        }else{
            value = std::get<0>(res.names[res_dx]);
            text.append(std::get<0>(res.names[res_dx]));
            text.append(L" 将不被替换");
        }
        res.values[res_dx] = value;
        if (t != replace_list.size()){
            text.append(L"，");
        }
    }
    text.append(L"\n\n是否确认？");

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
    emit CommitConfirmReplaceRes(res);
}

void CReplaceWindow::closeEvent(QCloseEvent* e)
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
        msgbox.setText(QString::fromStdWString(L"替换项尚未确认，如退出将被视为放弃。是否确认退出？"));
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

CReplaceWindow::~CReplaceWindow()
{
    disconnect(ui->ConfirmButton, SIGNAL(clicked()), this, SLOT(confirm_replace()));
    delete ui;
}
