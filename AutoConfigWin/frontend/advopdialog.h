#ifndef ADVOPDIALOG_H
#define ADVOPDIALOG_H

#include <QDialog>
#include <memory>
#include <QPushButton>
#include <QRadioButton>
#include <QButtonGroup>

namespace Ui {
class AdvOpDialog;
}

class AdvOpDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AdvOpDialog(QWidget *parent = nullptr);
    ~AdvOpDialog();

    std::unique_ptr<QButtonGroup> button_group;
    QPushButton* button_yes;
    QPushButton* button_no;
    QRadioButton* radio_button_1;
    QRadioButton* radio_button_2;
    QRadioButton* radio_button_3;

    void set_init_value(int op);

private:
    Ui::AdvOpDialog *ui;
};

#endif // ADVOPDIALOG_H
