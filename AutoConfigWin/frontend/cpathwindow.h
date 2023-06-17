#ifndef CPATHWINDOW_H
#define CPATHWINDOW_H

#include "../constants.h"
#include "creplacewindow.h"
#include <QWidget>
#include <QRadioButton>
#include <QButtonGroup>

namespace Ui {
class CPathWindow;
}

class CPathListItem: public QWidget
{
    Q_OBJECT

public:
    explicit CPathListItem(QWidget* parent = 0) {}
    std::unique_ptr<QLabel> title;
    std::unique_ptr<QRadioButton> button_cmd;
    std::unique_ptr<QRadioButton> button_cmd2;
    std::unique_ptr<QRadioButton> button_file;
    std::unique_ptr<QButtonGroup> button_group;
    std::unique_ptr<QLineEdit> filename;
    std::unique_ptr<QPushButton> button_select;
    std::unique_ptr<QLineEdit> cdname;
    std::unique_ptr<QPushButton> button_selectcd;
    int res_dx;

    void init(std::string bkgd_color, std::pair<std::wstring, std::wstring> path_str, int dx);
    ~CPathListItem();

private:
    std::wstring package_name;

public slots:
    void on_toggle(bool);
    void on_toggle2(bool);
    void select_dir();
    void select_cd();
};

class CPathList: public QListWidget
{
    Q_OBJECT

public:
    explicit CPathList(QListWidget *parent = 0);

    std::vector<std::unique_ptr<CPathListItem>> item_list;
    std::vector<std::unique_ptr<QListWidgetItem>> widget_items;
    void init(PathList path_list);
};

class CPathWindow : public QWidget
{
    Q_OBJECT

public:
    explicit CPathWindow(PathList path_list2, ReplaceList replace_list2, std::wstring paragraph, QWidget *parent = nullptr);
    ~CPathWindow();

    void init_graphics(PathList path_list2, ReplaceList replace_list2, std::wstring paragraph); //初始化主界面
    void closeEvent(QCloseEvent* e);

signals:
    void CommitConfirmPathRes(ConfirmPathRes path_res, ConfirmReplaceRes replace_res);
    void GiveUp();

public slots:
    void confirm_path();

private:
    Ui::CPathWindow *ui;
    std::unique_ptr<CPathList> c_path_list;
    std::unique_ptr<CReplaceList> c_replace_list;
    PathList path_list;
    ReplaceList replace_list;
    std::wstring raw_para;
    bool confirm_close = false;
};

#endif // CPATHWINDOW_H
