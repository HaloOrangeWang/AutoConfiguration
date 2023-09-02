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
    bool parse_first_answer(std::wstring origin_answer, std::wstring check_answer);

signals:
    void EmitGPTQuestion(std::wstring question_str); //根据问题解析结果，进一步向GPT提出问题
    void ConfirmInstall(InstallList package_name, std::wstring paragraph); //要求确认安装结果
    void ConfirmReplace(ReplaceList replace_question, std::wstring paragraph); //要求确认替换结果
    void ConfirmPath(PathList path_question, ReplaceList replace_question, std::wstring paragraph); //要求确认命令的执行路径和替换结果
    void ParseSuccess(std::vector<Opt> opt_list); //解析指令成功后的返回内容
    void ParseFailed(); //解析指令失败

public:
    void on_confirm_install(std::vector<ConfirmInstallRes> res);
    void on_confirm_replace(ConfirmReplaceRes res);
    void on_confirm_path(ConfirmPathRes path_res, ConfirmReplaceRes replace_res);
    std::vector<std::wstring> unmanageable_para_list;  //找不到对应命令的段落列表

private:
    std::vector<std::wstring> get_exec_paragraphs(std::wstring origin_answer, std::wstring check_answer); //将答案转换成若干要执行的段落。转换依据：1.根据换行符和序号拆分段落；2.删掉仅用于检查和校验的段落
    void parse_answer(); //处理回答
    std::vector<int> get_opt_type(std::vector<std::wstring> paragraphs); //根据GPT返回的每一条指令，解析指令类型为什么。
    std::tuple<bool, std::vector<Opt>, ReplaceList> parse_install_opt(std::wstring paragraph, ConfirmReplaceRes replace_res); //根据GPT返回的一条安装指令，解析它的内容
    std::tuple<std::vector<Opt>, PathList, ReplaceList> parse_exec_opt(std::wstring paragraph, ConfirmPathRes path_res, ConfirmReplaceRes replace_res);
    bool is_installed(std::wstring command); //检查一个包是否已经安装过了。command是检查是否已安装的完整的命令
    InstallList get_package_list(std::wstring paragraph); //获取要安装的包列表
    ReplaceList check_replace(std::wstring raw_command, std::wstring dscp); //检查安装命令中是否有需要替换的部分
    std::wstring replace_command(std::wstring raw_command, ConfirmReplaceRes replace_res);
    struct Section
    {
        // 从GPT返回内容解析出来的，一段命令的内容及其目的、执行地址等说明
        std::wstring aim; //这一条命令的目的
        std::wstring loc; //这一条命令执行的地点
        std::wstring content; //这一条命令的内容
        std::wstring mark; //这一条命令的说明
    };
    // 确定一个包是否已安装的标志位
    enum InstallFlag
    {
        Unconfirm = 0, //尚未确定是否已安装
        Installed, //已安装
        NotInstalled //未安装
    };

    std::vector<Section> split_sections(std::wstring paragraph); //将GPT返回的一条指令，进一步拆分成若干个小区段
    std::tuple<std::wstring, std::wstring> get_exec_path(std::wstring section); //获取这条命令在哪里执行
    std::wstring replace_path(std::wstring confirm_msg, ConfirmPathRes path_res); //确认文件路径的替换信息后，使用输入的替换结果作为最终的文件路径
    std::wstring del_special_char(std::wstring command_input); //删除特殊字符，如 >、注释行等
    std::vector<Opt> opt_process(std::vector<Opt> raw_opts); //对指令进行加工。如将apt-get替换为国内源，修改下载路径等
    std::wstring handle_powershell(std::wstring raw_command, std::wstring exec_path); //如果命令的执行地址中有“powershell”，那么在执行命令中需要增加“powershell -Command”。
    std::wstring convert_to_powershell(std::wstring raw_command); //对于需要在powershell中执行的指令，进行加工
    ParseResult parse_result; //最终的解析结果
    std::wstring env_os; //处理这个问题时所处的操作系统环境
    std::vector<std::wstring> packages_to_download; //存储要下载的包列表

    std::vector<ConfirmInstallRes> confirm_install_res;
    std::map<int, ConfirmReplaceRes> confirm_replace_res;
    std::map<int, ConfirmPathRes> confirm_path_res;

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

    CmdExec* cmd_exec;

public:
    AnswerParser(std::wstring os, boost::optional<std::wstring> package, CmdExec* cmd_exec_);
};

#endif // ANSWER_PARSER_H
