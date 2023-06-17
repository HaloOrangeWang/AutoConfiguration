#ifndef QUESTION_H
#define QUESTION_H

#include <string>
#include <vector>

class Question
{
public:
    virtual std::wstring format_question() = 0; //对问题进行整理，生成能够发送给GPT的字符串
    void send_question(std::wstring ques_str); //将问题发送给GPT

protected:
    // 背景-环境描述
    std::wstring env_os;
    std::vector<std::pair<std::wstring, std::wstring>> env_sw;
};

class QuestionInstall: public Question
{
public:
    std::wstring format_question();

protected:
    // 需要安装的内容
    std::wstring software_name; //需要安装的软件名

public:
    QuestionInstall(std::wstring env_os_2, std::vector<std::pair<std::wstring, std::wstring>> env_sw_2, std::wstring software_name_2): Question()
    {
        env_os = env_os_2;
        env_sw = env_sw_2;
        software_name = software_name_2;
    }
};

class QuestionConfigure: public Question
{
public:
    std::wstring format_question();

protected:
    // 需要配置的内容
    std::wstring software_name; //需要配置的软件名
    std::wstring software_path; //需要配置的软件的路径
    std::wstring description; //配置内容的描述

public:
    QuestionConfigure(std::wstring env_os_2, std::vector<std::pair<std::wstring, std::wstring>> env_sw_2, std::wstring software_name_2, std::wstring software_path_2, std::wstring description_2): Question()
    {
        env_os = env_os_2;
        env_sw = env_sw_2;
        software_name = software_name_2;
        software_path = software_path_2;
        description = description_2;
    }
};

#endif // QUESTION_H
