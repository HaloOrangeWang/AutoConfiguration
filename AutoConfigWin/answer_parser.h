#ifndef ANSWER_PARSER_H
#define ANSWER_PARSER_H

#include "constants.h"
#include "cmdexec.h"
#include <QObject>
#include <string>
#include <vector>
#include <boost/optional.hpp>
#include <boost/variant.hpp>

// using ConfirmPath = std::vector<std::pair<std::wstring, std::wstring>>;  //需要额外确认的路径问题。比如GPT回答“/folder”是你的xx软件的安装路径，那么就需要确认，xx软件的安装路径是什么。
using NeedConfirm = boost::variant<InstallList, ReplaceList>;

// 这个类的主要目的是，解析GPT返回的答案，并生成安装/配置的步骤
class AnswerParser: public QObject
{
    Q_OBJECT

public:
    struct ParseResult
    {
        bool is_success; //答案是否解析成功
        bool has_answer; //是否给出了最终的答案
        std::vector<Opt> opt_list; //最终的答案列表
    };
    bool parse_first_answer(std::wstring answer_str);
    bool parse_second_answer(std::wstring answer_str);

signals:
    void EmitGPTQuestion(std::wstring question_str); //根据问题解析结果，进一步向GPT提出问题
    void ConfirmInstall(InstallList package_name, std::wstring paragraph); //要求确认安装结果
    void ConfirmReplace(ReplaceList replace_question, std::wstring paragraph); //要求确认替换结果
    void ConfirmPath(PathList path_question, ReplaceList replace_question, std::wstring paragraph); //要求确认命令的执行路径和替换结果
    void ParseSuccess(std::vector<Opt> opt_list, std::wstring raw_answer); //解析指令成功后的返回内容
    void ParseFailed(); //解析指令失败

public:
    void on_confirm_install(std::vector<ConfirmInstallRes> res);
    void on_confirm_replace(ConfirmReplaceRes res);
    void on_confirm_path(ConfirmPathRes path_res, ConfirmReplaceRes replace_res);
    std::vector<std::wstring> unmanageable_para_list;  //找不到对应命令的段落列表

private:
    std::vector<std::wstring> split_paragraphs(std::wstring answer_str); //将字符串拆分成多个段落。拆分的依据为，换行符\n\n，以及 1. 这样的步骤id
    void parse_answer(); //处理回答
    std::vector<int> get_opt_type(std::vector<std::wstring> paragraphs); //根据GPT返回的每一条指令，解析指令类型为什么。
    std::tuple<std::wstring, Opt, std::wstring> parse_download_opt(std::wstring paragraph); //根据GPT返回的一条下载指令，解析它的内容
    std::tuple<bool, std::vector<Opt>, NeedConfirm> parse_install_opt(std::wstring paragraph2, ConfirmReplaceRes replace_res); //根据GPT返回的一条安装指令，解析它的内容
    std::tuple<std::vector<Opt>, PathList, ReplaceList> parse_exec_opt(std::wstring paragraph2, ConfirmPathRes path_res, ConfirmReplaceRes replace_res);
    std::wstring remove_example(std::wstring paragraph); //删除一段话中“例如”后面的内容
    bool is_installed(std::wstring package_name); //检查一个包是否已经被安装过了
    int is_confirm_install(std::wstring package_name); //检查一个包是否已经被确认过安装
    InstallList get_package_list(std::wstring paragraph); //获取要安装的包列表
    ReplaceList check_replace(std::wstring raw_command, std::wstring dscp); //检查安装命令中是否有需要替换的部分
    std::wstring replace_command(std::wstring raw_command, ConfirmReplaceRes replace_res);
    std::vector<std::pair<std::wstring, bool>> split_sections(std::wstring paragraph); //将GPT返回的一条指令，进一步拆分成若干个小区段
    std::tuple<std::wstring, std::wstring> get_exec_path(std::wstring section); //获取这条命令在哪里执行
    std::wstring replace_path(std::wstring confirm_msg, ConfirmPathRes path_res); //确认文件路径的替换信息后，使用输入的替换结果作为最终的文件路径
    int find_not_check_ed_dx(std::vector<std::pair<std::wstring, bool>> sections); //排除掉用于校验的段落。返回最后一个非校验段落在sections中的索引值
    std::wstring del_special_char(std::wstring command_input); //删除特殊字符，如 >、注释行等
    std::vector<Opt> opt_process(std::vector<Opt> raw_opts); //对指令进行加工。如将apt-get替换为国内源，修改下载路径等
    ParseResult parse_result; //最终的解析结果
    std::wstring env_os; //处理这个问题时所处的操作系统环境
    boost::optional<std::wstring> install_package; //处理这个问题时所处的操作系统环境
    std::vector<std::wstring> packages_to_download; //存储要下载的包列表

    std::vector<ConfirmInstallRes> confirm_install_res;
    std::map<int, ConfirmReplaceRes> confirm_replace_res;
    std::map<int, ConfirmPathRes> confirm_path_res;

    std::wstring raw_answer_ws;
    CmdExec* cmd_exec;

    struct ParaTree
    {
        std::wstring text;
        int opt_type;
        boost::optional<int> parent_node;
        boost::optional<int> brother_node;
        ParaTree(std::wstring text2, int opt_type2, boost::optional<int> parent_node2, boost::optional<int> brother_node2): text(text2), opt_type(opt_type2), parent_node(parent_node2), brother_node(brother_node2){}
    };
    std::vector<ParaTree> para_tree;
    int curr_node = 0;
    int num_question = 0;

public:
    AnswerParser(std::wstring os, boost::optional<std::wstring> package, CmdExec* cmd_exec_);
};

#endif // ANSWER_PARSER_H
