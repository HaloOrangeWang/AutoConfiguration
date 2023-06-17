#ifndef CMDLISTVIEW_H
#define CMDLISTVIEW_H

#include "../constants.h"
#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QRadioButton>
#include <QButtonGroup>
#include <QPushButton>
#include <QListWidget>
#include <memory>
#include <boost/variant.hpp>

enum OverwriteStatus
{
    NotChosen = 0,
    Overwrite,
    Append
};

struct RealOpt
{
    Opt opt;
    OverwriteStatus overwrite_status;
    bool modified;
};

class CmdListView;
class CmdListItem: public QWidget
{
    Q_OBJECT

public:
    std::unique_ptr<QLabel> title;  //表明每一条指令的序号、类型
    std::unique_ptr<QLabel> path_msg; //表明每一条指令的路径
    std::unique_ptr<QLineEdit> path_text; //展示执行地址的view
    std::unique_ptr<QLabel> content_msg;  //表明每一条指令的具体内容
    std::unique_ptr<QTextEdit> content_text; //展示每一条指令内容的view
    std::unique_ptr<QRadioButton> button_overwrite; //表明文件应该覆写的按钮
    std::unique_ptr<QRadioButton> button_append; //表明文件应该追加的按钮
    std::unique_ptr<QButtonGroup> button_group;

    std::unique_ptr<QPushButton> button_edit; //编辑按钮
    std::unique_ptr<QPushButton> button_del; //删除按钮
    std::unique_ptr<QPushButton> button_up; //上移按钮
    std::unique_ptr<QPushButton> button_down; //下移按钮
    std::unique_ptr<QPushButton> button_save; //保存按钮
    std::unique_ptr<QPushButton> button_nosave; //不保存按钮    
    int idx;

    void init(std::string bkgd_color, RealOpt opt, int idx2);
    ~CmdListItem();

public slots:
    void on_edit(); //启动编辑后执行的内容
    void on_save(); //保存后执行的内容
    void on_nosave(); //决定不保存后执行的内容
    void on_up(); //点击上移的按钮
    void on_down(); //点击下移的按钮
    void on_del(); //点击删除的按钮
    void on_choose_overwrite(bool); //选择了“覆盖文件”
    void on_choose_append(bool); //选择了“追加到末尾”

private:
    CmdListView* pview;

public:
    explicit CmdListItem(CmdListView* pview2, QWidget* parent = 0): pview(pview2) {}
};

class CmdListView: public QListWidget
{
    Q_OBJECT

public:
    explicit CmdListView(QListWidget *parent = 0);

    void init(std::vector<Opt> opt_list);
    boost::variant<RealCmdList, std::wstring> submit();

    void on_swap(int dx1, int dx2); //交换两条命令
    void on_del(int dx); //删除一条命令

    int command_num;
    std::vector<RealOpt> real_opt_list;

public slots:
    void add_newcmd();

private:
    void rerander();
};

class RawGPTWindow : public QWidget
{
    Q_OBJECT

public:
    std::unique_ptr<QTextEdit> textview; //展示原始GPT返回内容的view
    void init(std::wstring gpt_content);
};

#endif // CMDLISTVIEW_H
