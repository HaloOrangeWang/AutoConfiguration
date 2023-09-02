#pragma execution_character_set("utf-8")

#include "question.h"
#include "constants.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

std::wstring format_install_question(std::wstring env_os, std::vector<std::pair<std::wstring, std::wstring>> env_sw, std::wstring package, int venv_option)
{
    boost::property_tree::wptree root, env_sw_2;

    root.put(L"env_os", env_os);
    for (int t = 0; t < env_sw.size(); t++){
        boost::property_tree::wptree env_sw_3;
        env_sw_3.put(L"", env_sw[t].first);
        env_sw_2.push_back(std::make_pair(L"", env_sw_3));
    }
    root.add_child(L"env_sw", env_sw_2);
    root.put(L"type", OrderInstall);
    root.put(L"package", package);
    root.put(L"venv_option", venv_option);
    root.put(L"version", VERSION);

    std::wostringstream oss;
    boost::property_tree::write_json(oss, root);
    return oss.str();
}

std::wstring format_config_question(std::wstring env_os, std::vector<std::pair<std::wstring, std::wstring>> env_sw, std::wstring package, std::wstring description, int venv_option)
{
    boost::property_tree::wptree root, env_sw_2;

    root.put(L"env_os", env_os);
    for (int t = 0; t < env_sw.size(); t++){
        boost::property_tree::wptree env_sw_3;
        env_sw_3.put(L"", env_sw[t].first);
        env_sw_2.push_back(std::make_pair(L"", env_sw_3));
    }
    root.add_child(L"env_sw", env_sw_2);
    root.put(L"type", OrderConfig);
    root.put(L"package", package);
    root.put(L"description", description);
    root.put(L"venv_option", venv_option);
    root.put(L"version", VERSION);

    std::wostringstream oss;
    boost::property_tree::write_json(oss, root);
    return oss.str();
}
