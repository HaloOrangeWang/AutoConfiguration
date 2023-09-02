#pragma execution_character_set("utf-8")

#include "advopdialog.h"
#include "ui_advopdialog.h"
#include "../constants.h"

AdvOpDialog::AdvOpDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AdvOpDialog)
{
    ui->setupUi(this);

    button_yes = ui->button_yes;
    button_no = ui->button_no;
    radio_button_1 = ui->radioButton1;
    radio_button_2 = ui->radioButton2;
    radio_button_3 = ui->radioButton3;

    // 初始化按钮行为
    button_group = std::make_unique<QButtonGroup>(this);
    button_group->addButton(ui->radioButton1, 0);
    button_group->addButton(ui->radioButton2, 0);
    button_group->addButton(ui->radioButton3, 0);
    ui->radioButton1->setChecked(true);
}

void AdvOpDialog::set_init_value(int op)
{
    // 初始化被选中的按钮
    if (op == VenvForbid){
        ui->radioButton1->setChecked(true);
    }else if (op == VenvAllow){
        ui->radioButton2->setChecked(true);
    }else{
        ui->radioButton3->setChecked(true);
    }
}

AdvOpDialog::~AdvOpDialog()
{
    delete ui;
}
