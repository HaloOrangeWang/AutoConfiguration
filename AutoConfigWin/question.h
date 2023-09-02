#ifndef QUESTION_H
#define QUESTION_H

#include <string>
#include <vector>

std::wstring format_install_question(std::wstring env_os, std::vector<std::pair<std::wstring, std::wstring>> env_sw, std::wstring package, int venv_option);
std::wstring format_config_question(std::wstring env_os, std::vector<std::pair<std::wstring, std::wstring>> env_sw, std::wstring package, std::wstring description, int venv_option);

#endif // QUESTION_H
