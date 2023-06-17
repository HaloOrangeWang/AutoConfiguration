#ifndef CREPLACEWINDOW_H
#define CREPLACEWINDOW_H

#include "../constants.h"
#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include <QRadioButton>
#include <QButtonGroup>
#include <QPushButton>
#include <QListWidget>
#include <QCloseEvent>
#include <memory>
#include <tuple>

namespace Ui {
class CReplaceWindow;
}

class CReplaceListItem: public QWidget
{
    Q_OBJECT

public:
    explicit CReplaceListItem(QWidget* parent = 0) {}
    std::unique_ptr<QLabel> title;
    std::unique_ptr<QRadioButton> button_no_replace;
    std::unique_ptr<QRadioButton> button_need_replace;
    std::unique_ptr<QButtonGroup> button_group;
    std::unique_ptr<QLineEdit> pathname;
    std::unique_ptr<QPushButton> button_select;
    std::unique_ptr<QPushButton> button_select2;
    int res_dx;

    void init(std::string bkgd_color, std::tuple<std::wstring, std::wstring, std::wstring> replace_str, int dx);
    ~CReplaceListItem();

public slots:
    void on_toggle(bool);
    void select_path();
    void select_path2();
};

class CReplaceList: public QListWidget
{
    Q_OBJECT

public:
    explicit CReplaceList(QListWidget *parent = 0);

    std::vector<std::unique_ptr<CReplaceListItem>> item_list;
    std::vector<std::unique_ptr<QListWidgetItem>> widget_items;
    void init(ReplaceList replace_list);
};

class CReplaceWindow : public QWidget
{
    Q_OBJECT

public:
    explicit CReplaceWindow(ReplaceList replace_list2, std::wstring paragraph, QWidget *parent = nullptr);
    ~CReplaceWindow();

    void init_graphics(ReplaceList replace_list2, std::wstring paragraph); //初始化主界面
    void closeEvent(QCloseEvent* e);

signals:
    void CommitConfirmReplaceRes(ConfirmReplaceRes res);
    void GiveUp();

public slots:
    void confirm_replace();

private:
    Ui::CReplaceWindow *ui;
    std::unique_ptr<CReplaceList> c_replace_list;
    ReplaceList replace_list;
    std::wstring raw_para;
    bool confirm_close = false;
};

#endif // CREPLACEWINDOW_H
