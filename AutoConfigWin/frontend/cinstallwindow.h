#ifndef CINSTALLWINDOW_H
#define CINSTALLWINDOW_H

#include "../constants.h"
#include <QWidget>
#include <QListWidget>
#include <QRadioButton>
#include <QButtonGroup>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QCloseEvent>
#include <memory>

namespace Ui {
class CInstallWindow;
}

class CInstallListItem: public QWidget
{
    Q_OBJECT

public:
    explicit CInstallListItem(QWidget* parent = 0) {}
    std::unique_ptr<QLabel> title;
    std::unique_ptr<QRadioButton> button_installed;
    std::unique_ptr<QRadioButton> button_need_install;
    std::unique_ptr<QButtonGroup> button_group;
    std::unique_ptr<QLineEdit> dirname;
    std::unique_ptr<QPushButton> button_select;

    void init(std::string bkgd_color, std::wstring package_name2);
    ~CInstallListItem();

private:
    std::wstring package_name;

public slots:
    void on_toggle(bool);
    void select_dir();
};

class CInstallList: public QListWidget
{
    Q_OBJECT

public:
    explicit CInstallList(QListWidget *parent = 0);

    std::vector<std::unique_ptr<CInstallListItem>> item_list;
    std::vector<std::unique_ptr<QListWidgetItem>> widget_items;
    void init(InstallList install_list);
};

class CInstallWindow : public QWidget
{
    Q_OBJECT

public:
    explicit CInstallWindow(InstallList install_list2, std::wstring paragraph, QWidget *parent = nullptr);
    ~CInstallWindow();

    void init_graphics(InstallList install_list2, std::wstring paragraph); //初始化主界面
    void closeEvent(QCloseEvent* e);

signals:
    void CommitConfirmInstallRes(std::vector<ConfirmInstallRes> res);
    void GiveUp();

public slots:
    void confirm_install();

private:
    Ui::CInstallWindow *ui;
    std::unique_ptr<CInstallList> c_install_list;
    InstallList install_list;
    bool confirm_close = false;
};

#endif // CINSTALLWINDOW_H
