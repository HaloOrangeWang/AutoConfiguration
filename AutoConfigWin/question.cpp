#pragma execution_character_set("utf-8")

#include "question.h"

std::wstring QuestionInstall::format_question()
{
    std::wstring output;
    output = L"在";
    output.append(env_os);
    for (int t = 0; t < env_sw.size(); t++){
        output.append(L"，");
        output.append(env_sw[t].first);
    }
    output.append(L"环境下，使用命令行安装");
    output.append(software_name);
    output.append(L"。\n\n1. 最好使用一种包版本管理工具\n\n2. 请明确给出每一步操作需要输入的的命令，不需要举例子。\n\n3. 完成后不需要使用它，也不需要验证安装结果。");
    return output;
}

std::wstring QuestionConfigure::format_question()
{
    std::wstring output;
    output = L"在";
    output += env_os;
    for (int t = 0; t < env_sw.size(); t++){
        output.append(L"，");
        output.append(env_sw[t].first);
    }
    output.append(L"环境下，使用命令行配置");
    output.append(software_name);
    output.append(description);
    return output;
}
