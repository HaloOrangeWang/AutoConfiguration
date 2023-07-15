#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <string>
#include <vector>
#include <boost/optional.hpp>

const std::string ServerURL = "ws://43.133.10.205:58000/submit"; //向GPT提问的websocket地址
const std::string ReportURL = "http://43.133.10.205:58000/report"; //反馈bug和提出改进意见的地址
//const std::string ServerURL = "ws://127.0.0.1:58000/submit";

const std::vector<wchar_t> Puncs = {L'，', L'。', L'：', L'；', L'\n'}; //可用于断句的标点符号列表
const std::vector<wchar_t> ChnQuotations = {L'‘', L'’', L'“', L'”'}; //中文引号列表

enum OptType  //操作类型。1为下载，2为安装，3为执行命令，4为校验
{
    Ignore = 0,
    Download,
    Install,
    Exec,
    Verify,
    Unmanageable
};

struct Opt  //一条操作指令
{
    int opt_type; //命令类型
    std::wstring file_path; //在哪个文件操作。空字符串表示在命令行操作
    std::wstring command; //操作内容
    Opt(){}
    Opt(int opt_type2, std::wstring file_path2, std::wstring command2): opt_type(opt_type2), file_path(file_path2), command(command2){}
};

using InstallList = std::vector<std::wstring>;  //需要额外确认的安装问题。比如GPT回答“需要安装vcpkg“，那么程序就应该确认vcpkg是否已经安装过了。
struct ConfirmInstallRes //确认XX库是否已安装过的指令
{
    std::wstring package_name; //库名
    boost::optional<std::wstring> path; //库的安装路径
};

using ReplaceList = std::vector<std::tuple<std::wstring, std::wstring, std::wstring>>;
struct ConfirmReplaceRes
{
    ReplaceList names; //需要替换的原始字段名，以及它们的别称
    std::vector<std::wstring> values; //每个字段名的真实数值
};

using PathList = std::vector<std::pair<std::wstring, std::wstring>>;
struct ConfirmPathRes
{
    PathList names; //需要确认的文件名，以及它们的路径
    std::vector<std::wstring> values; //每个文件名的真实路径。空字符串表示命令行，cd:xxx表示在命令行中cd到某一目录下执行。
};

struct RealCmd
{
    std::wstring path; //写入的文件文件路径
    bool overwrite; //是否覆盖
    std::wstring command; //写入/执行的内容
};
using RealCmdList = std::vector<RealCmd>;  //正式执行的命令列表

// 一条命令执行的结果
struct ExecRes
{
    int return_value; //命令执行的返回值
    std::wstring out_str; //这条命令在stdout的结果
};

#endif // CONSTANTS_H
