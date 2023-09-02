#pragma execution_character_set("utf-8")

#ifndef FUNCS_H
#define FUNCS_H

#include "constants.h"
#include <string>
#include <Windows.h>

inline bool is_eng_number(wchar_t c)
{
    if (c >= 0x30 && c <= 0x39){
        return true;
    }
    if (c >= 0x41 && c <= 0x5a){
        return true;
    }
    if (c >= 0x61 && c <= 0x7a){
        return true;
    }
    return false;
}

inline bool is_1byte_word(wchar_t c)
{
    if (c >= 0x20 && c <= 0x7f){
        return true;
    }
    return false;
}

inline bool is_chn_chr(wchar_t c)
{
    if (c >= 0x4e00 && c <= 0x9fff){
        return true;
    }
    return false;
}

inline bool is_eng_chr(wchar_t c)
{
    if (c >= 0x41 && c <= 0x5a){
        return true;
    }
    if (c >= 0x61 && c <= 0x7a){
        return true;
    }
    return false;
}

inline bool is_capital_eng_chr(wchar_t c)
{
    if (c >= 0x41 && c <= 0x5a){
        return true;
    }
    return false;
}

inline bool has_chn_chr(std::wstring s)
{
    for (int t = 0; t < s.size(); t++){
        if (s[t] >= 0x4e00 && s[t] <= 0x9fff){
            return true;
        }
    }
    return false;
}

inline std::string UnicodeToUTF8(const std::wstring& s)
{
    int nLen = ::WideCharToMultiByte(CP_UTF8, 0, s.c_str(), -1, nullptr, 0, nullptr, nullptr);

    char* buffer = new char[nLen + 1];
    ::ZeroMemory(buffer, nLen + 1);

    ::WideCharToMultiByte(CP_UTF8, 0, s.c_str(), -1, buffer, nLen, nullptr, nullptr);

    std::string multStr = buffer;
    delete[] buffer;
    return multStr;
}

inline std::wstring UTF8ToUnicode(const std::string &s)
{
    std::wstring result;

    // 获得缓冲区的宽字符个数
    int length = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, NULL, 0);

    wchar_t * buffer = new wchar_t[length];
    ::MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, buffer, length);
    result = buffer;

    delete[] buffer;
    return result;
}

//将一个字符串开头和结尾的空格和换行符去掉
inline std::wstring trim_n(std::wstring s)
{
    int st_dx = -1;
    int ed_dx = -1;
    for (int t = 0; t < s.size(); t++){
        if (s[t] != L'\n' && s[t] != L' '){
            st_dx = t;
            break;
        }
    }
    for (int t = static_cast<int>(s.size()) - 1; t >= 0; t--){
        if (s[t] != L'\n' && s[t] != L' '){
            ed_dx = t;
            break;
        }
    }
    if (st_dx == -1 || ed_dx == -1){
        return std::wstring();
    }else{
        return s.substr(st_dx, ed_dx - st_dx + 1);
    }
}

// 将一个字符串开头的空格、特殊字符，以及结尾的空格的空格、换行符去掉
inline std::wstring trim_n_spechar(std::wstring s)
{
    int st_dx = -1;
    int ed_dx = -1;
    for (int t = 0; t < s.size(); t++){
        if (is_eng_number(s[t]) || is_chn_chr(s[t])){
            st_dx = t;
            break;
        }
    }
    for (int t = static_cast<int>(s.size()) - 1; t >= 0; t--){
        if (s[t] != L'\n' && s[t] != L' '){
            ed_dx = t;
            break;
        }
    }
    if (st_dx == -1 || ed_dx == -1){
        return std::wstring();
    }else{
        return s.substr(st_dx, ed_dx - st_dx + 1);
    }
}

// 获取一段话中的第一个句子，以中文标点符号或换行符作为第一句的结尾。但是
inline std::wstring get_first_sentence(std::wstring s)
{
    std::wstring sentence;
    for (int t = 0; t < s.size(); t++){
        if (std::find(Puncs.begin(), Puncs.end(), s[t]) != Puncs.end()){
            break;
        }else{
            sentence.push_back(s[t]);
        }
    }
    return sentence;
}

// 检查目标字符串是否在另一个大字符串中。如果在，则返回目标字符串，否则返回为空
// 另注：如果目标字符串以标点符号作为开头结尾，但标点符号
inline std::wstring find_in_command(std::wstring sentence, std::wstring target)
{
    if (sentence.find(target) != std::wstring::npos){
        return target;
    }
    if (target.size() <= 1){
        return std::wstring();
    }
    int end_chr_dx = target.size() - 1;
    if ((target[0] == L'{' && target[end_chr_dx] == L'}') ||
        (target[0] == L'[' && target[end_chr_dx] == L']') ||
        (target[0] == L'(' && target[end_chr_dx] == L')') ||
        (target[0] == L'"' && target[end_chr_dx] == L'"') ||
        (target[0] == L'\'' && target[end_chr_dx] == L'\'') ||
        (target[0] == L'“' && target[end_chr_dx] == L'”') ||
        (target[0] == L'‘' && target[end_chr_dx] == L'’') ||
        (target[0] == L'`' && target[end_chr_dx] == L'`')){
        std::wstring target_inner = target.substr(1, target.size() - 2);
        if (sentence.find(target_inner) != std::wstring::npos){
            return target_inner;
        }
    }
    return std::wstring();
}

inline std::wstring remove_mark(std::wstring str)
{
    if (str.size() <= 1){
        return str;
    }
    int end_chr_dx = str.size() - 1;
    if ((str[0] == L'{' && str[end_chr_dx] == L'}') ||
        (str[0] == L'[' && str[end_chr_dx] == L']') ||
        (str[0] == L'(' && str[end_chr_dx] == L')') ||
        (str[0] == L'"' && str[end_chr_dx] == L'"') ||
        (str[0] == L'\'' && str[end_chr_dx] == L'\'') ||
        (str[0] == L'“' && str[end_chr_dx] == L'”') ||
        (str[0] == L'‘' && str[end_chr_dx] == L'’') ||
        (str[0] == L'`' && str[end_chr_dx] == L'`')){
        return str.substr(1, str.size() - 2);
    }
    return str;
}

// 获取一句话中的最后一个英文词。例如：https://www.google.com/的最后一个词是com
inline std::wstring get_last_word(std::wstring str)
{
    if (str.empty()){
        return L"";
    }
    std::wstring str_output;
    bool has_word = false;
    for (int t = str.size() - 1; t >= 0; t--){
        if (is_eng_number(str[t])){
            str_output.insert(str_output.begin(), str[t]);
            has_word = true;
        }else{
            if (has_word){
                break;
            }
        }
    }
    return str_output;
}

// 将OptType转换为字符串
inline std::wstring get_opt_type_str(int opt_type)
{
    if (opt_type == Ignore){
        return L"不需要处理";
    }else if (opt_type == Install){
        return L"安装";
    }else if (opt_type == Exec){
        return L"执行命令";
    }
    return L"未知";
}

#endif // FUNCS_H
