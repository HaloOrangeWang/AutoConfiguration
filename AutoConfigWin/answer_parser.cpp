#pragma execution_character_set("utf-8")

#include "answer_parser.h"
#include "funcs.h"
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/regex.hpp>
#include <boost/filesystem.hpp>
#include <boost/regex.hpp>
#include <functional>
#include <QDebug>

AnswerParser::AnswerParser(std::wstring os, boost::optional<std::wstring> package, CmdExec* cmd_exec_)
{
    env_os = os;
    if (package.has_value()){
        install_package = package.value();
    }else{
        install_package = boost::none;
    }
    cmd_exec = cmd_exec_;
}

bool AnswerParser::parse_first_answer(std::wstring answer_str)
{
    num_question = 1;
    // 1.先将原始答案分段
    raw_answer_ws = answer_str;
    std::wstring answer_str2 = trim_between_n(answer_str);
    std::vector<std::wstring> paragraphs = split_paragraphs(answer_str2);
    if (paragraphs.size() == 0){
        return false;
    }
    // 2.针对每个段落，分析这个段落对应的执行类型，并构建执行树
    std::vector<int> opt_type_list = get_opt_type(paragraphs);
    for (int para_it = 0; para_it < paragraphs.size(); para_it++){
        if (para_it != paragraphs.size() - 1){
            para_tree.push_back(ParaTree(paragraphs[para_it], opt_type_list[para_it], boost::none, para_it + 1));
        }else{
            para_tree.push_back(ParaTree(paragraphs[para_it], opt_type_list[para_it], boost::none, boost::none));
        }
    }
    curr_node = 0;
    // 3.处理分段后的GPT返回结果
    parse_answer();
    return true;
}

bool AnswerParser::parse_second_answer(std::wstring answer_str)
{
    num_question += 1;
    if (num_question >= 10){
        return false;
    }
    // 1.先将答案分段
    std::wstring answer_str2 = trim_between_n(answer_str);
    std::vector<std::wstring> paragraphs = split_paragraphs(answer_str2);
    if (paragraphs.size() == 0){
        return false;
    }
    // 2.针对每个段落，分析这个段落对应的执行类型，并构建执行树
    std::vector<int> opt_type_list = get_opt_type(paragraphs);
    int parent_node = curr_node;
    int next_node = para_tree.size();
    for (int para_it = 0; para_it < paragraphs.size(); para_it++){
        if (para_it != paragraphs.size() - 1){
            para_tree.push_back(ParaTree(paragraphs[para_it], opt_type_list[para_it], parent_node, para_tree.size() + 1));
        }else{
            para_tree.push_back(ParaTree(paragraphs[para_it], opt_type_list[para_it], parent_node, boost::none));
        }
    }
    curr_node = next_node;
    // 3.处理分段后的GPT返回结果
    parse_answer();
    return true;
}

void AnswerParser::on_confirm_install(std::vector<ConfirmInstallRes> res)
{
    confirm_install_res = res;
    parse_answer();
}

void AnswerParser::on_confirm_replace(ConfirmReplaceRes res)
{
    confirm_replace_res.insert(std::make_pair(curr_node, res));
    parse_answer();
}

void AnswerParser::on_confirm_path(ConfirmPathRes path_res, ConfirmReplaceRes replace_res)
{
    confirm_path_res.insert(std::make_pair(curr_node, path_res));
    confirm_replace_res.insert(std::make_pair(curr_node, replace_res));
    parse_answer();
}

void AnswerParser::parse_answer()
{
    while (true){
        int opt_type = para_tree[curr_node].opt_type;
        // 1.针对这一段话，获取命令执行的内容，以及命令执行的方法
        if (opt_type == Ignore){
            // 如果这条指令不需要处理，那么直接忽略即可
        }else if (opt_type == Download){
            // 处理下载指令
            std::tuple<std::wstring, Opt, std::wstring> res = parse_download_opt(para_tree[curr_node].text);
            packages_to_download.push_back(std::get<0>(res));
            if (std::get<2>(res).empty()){
                parse_result.opt_list.push_back(std::get<1>(res));
            }else{
                // 对于不明确的指令，可以不执行
                // TODO: 未来对于不明确的指令，需要继续提问
                // emit EmitGPTQuestion(std::get<2>(res));
                qDebug() << "第" << curr_node + 1 << "号命令，类型为下载，但没有解析出下载地址";
                // return;
            }
        }else if (opt_type == Install){
            // 处理安装指令
            ConfirmReplaceRes confirm_res_curnode{};
            if (confirm_replace_res.find(curr_node) != confirm_replace_res.end()){
                confirm_res_curnode = confirm_replace_res.at(curr_node);
            }
            std::tuple<bool, std::vector<Opt>, NeedConfirm> res = parse_install_opt(para_tree[curr_node].text, confirm_res_curnode);
            if (std::get<0>(res) == true){
                NeedConfirm confirm = std::get<2>(res);
                if (confirm.type() == typeid(InstallList)){
                    InstallList install_q = boost::get<InstallList>(confirm);
                    if (install_q.empty()){
                        std::vector<Opt> opt_list = std::get<1>(res);
                        for (auto it = opt_list.begin(); it != opt_list.end(); it++){
                            parse_result.opt_list.push_back(*it);
                        }
                    }else{
                        emit ConfirmInstall(install_q, para_tree[curr_node].text);
                        return;
                    }
                }else{
                    ReplaceList replace_q = boost::get<ReplaceList>(confirm);
                    if (replace_q.empty()){
                        std::vector<Opt> opt_list = std::get<1>(res);
                        for (auto it = opt_list.begin(); it != opt_list.end(); it++){
                            parse_result.opt_list.push_back(*it);
                        }
                    }else{
                        emit ConfirmReplace(replace_q, para_tree[curr_node].text);
                        return;
                    }
                }
            }else{
                // todo: emit解析失败的指令
            }
        }else if (opt_type == Exec){
            // 处理执行命令的指令
            ConfirmPathRes confirm_path_res_curnode{};
            ConfirmReplaceRes confirm_replace_res_curnode{};
            if (confirm_path_res.find(curr_node) != confirm_path_res.end()){
                confirm_path_res_curnode = confirm_path_res.at(curr_node);
            }
            if (confirm_replace_res.find(curr_node) != confirm_replace_res.end()){
                confirm_replace_res_curnode = confirm_replace_res.at(curr_node);
            }
            std::tuple<std::vector<Opt>, PathList, ReplaceList> res = parse_exec_opt(para_tree[curr_node].text, confirm_path_res_curnode, confirm_replace_res_curnode);
            PathList path_q = std::get<1>(res);
            ReplaceList replace_q = std::get<2>(res);
            if (path_q.size() == 0 && replace_q.size() == 0){
                // 没有任何需要进一步确认的内容
                std::vector<Opt> opt_list = std::get<0>(res);
                for (auto it = opt_list.begin(); it != opt_list.end(); it++){
                    parse_result.opt_list.push_back(*it);
                }
            }else{
                emit ConfirmPath(path_q, replace_q, para_tree[curr_node].text);
                return;
            }
        }else if (opt_type == Unmanageable){
            unmanageable_para_list.push_back(para_tree[curr_node].text);
        }

        // 2.判断要执行的下一段话是什么
        while (true){
            if (para_tree[curr_node].brother_node.has_value()){
                curr_node = para_tree[curr_node].brother_node.value();
                break;
            }else if (para_tree[curr_node].parent_node.has_value()){
                curr_node = para_tree[curr_node].parent_node.value();
            }else{
                // 某个段落既没有父级段落，又没有“下一个”段落，说明已经完成了整篇回答的解析。此时直接返回解析结果即可。
                parse_result.opt_list = opt_process(parse_result.opt_list);
                emit ParseSuccess(parse_result.opt_list, raw_answer_ws);
                return;
            }
        }
    }
}

std::vector<std::wstring> AnswerParser::split_paragraphs(std::wstring answer_str)
{
    // 1.将答案按照'\n'拆分
    std::vector<std::wstring> raw_paragraphs;
    std::vector<std::wstring> paragraphs;
    boost::split_regex(raw_paragraphs, answer_str, boost::wregex(L"\n"));
    // 2.按照命令id进行组合
    // 如果这行内容以 a. 开头（a为当前command_id），则说明它开启了一个新命令，否则说明它不是新命令的起始
    boost::wregex regstr(L"^(?<id2>[0-9]+)\\.(.*)");
    int command_id = 1;
    for (int para_it = 0; para_it < raw_paragraphs.size(); para_it++){
        boost::wsmatch match_res;
        if (boost::regex_match(raw_paragraphs[para_it], match_res, regstr)){
            int command_id2 = stoi(match_res.str(L"id2"));
            if (command_id2 == command_id){
                paragraphs.push_back(raw_paragraphs[para_it]);
                command_id += 1;
            }else{
                //如果返回内容以 a. 开头，但a不是当前command_id，说明出现了问题，直接返回空内容
                return {};
            }
        }else{
            if (paragraphs.size() >= 1){
                paragraphs[paragraphs.size() - 1].append(L"\n");
                paragraphs[paragraphs.size() - 1].append(raw_paragraphs[para_it]);
            }
        }
    }
    if (paragraphs.empty()){
        paragraphs = {answer_str};
    }
    return paragraphs;
}

std::vector<int> AnswerParser::get_opt_type(std::vector<std::wstring> paragraphs)
{
    std::vector<int> opt_type_list(paragraphs.size(), 0);
    // 针对每个段落，确定它执行的命令类型是什么
    for (int t = 0; t < paragraphs.size(); t++){
        std::wstring paragraph = paragraphs[t];
        std::wstring first_sentence;
        if (paragraph.find(L"\n") != std::wstring::npos){
            first_sentence = paragraph.substr(0, paragraph.find(L"\n"));
        }else{
            first_sentence = paragraph;
        }
        if (first_sentence.find(L"等待") != std::wstring::npos){
            opt_type_list[t] = Ignore;
            continue;
        }
        if (first_sentence.find(L"安装") != std::wstring::npos && first_sentence.find(L"安装") <= 10){
            opt_type_list[t] = Install;
            continue;
        }
        if (first_sentence.find(L"下载") != std::wstring::npos && first_sentence.find(L"下载") <= 10){
            opt_type_list[t] = Download;
            continue;
        }
        if (paragraph.find(L"```") != std::wstring::npos){
            opt_type_list[t] = Exec;
            continue;
        }
        std::vector<std::wstring> ignore_keywords = {L"命令行", L"提示符", L"powershell", L"terminal", L"shell"};
        bool is_confirm2 = false;
        if (paragraph.find(L"打开") != std::wstring::npos){
            for (auto it = ignore_keywords.begin(); it != ignore_keywords.end(); it++){
                if (boost::to_lower_copy(paragraph).find(*it) != std::wstring::npos){
                    opt_type_list[t] = Ignore;
                    is_confirm2 = true;
                    break;
                }
            }
        }
        if (is_confirm2){
            continue;
        }
        opt_type_list[t] = Unmanageable;
        continue;
    }
    // 剔除出用于校验的命令
    for (int t = paragraphs.size() - 1; t >= 0; t--){
        std::wstring paragraph = paragraphs[t];
        std::wstring first_sentence;
        if (paragraph.find(L"\n") != std::wstring::npos){
            first_sentence = paragraph.substr(0, paragraph.find(L"\n"));
        }else{
            first_sentence = paragraph;
        }
        std::vector<std::wstring> verify_keywords = {L"验证", L"校验", L"检查", L"验证", L"测试"};
        bool is_confirm = false;
        for (auto it = verify_keywords.begin(); it != verify_keywords.end(); it++){
            if (first_sentence.find(*it) != std::wstring::npos){
                opt_type_list[t] = Verify;
                is_confirm = true;
                break;
            }
        }
        if (!is_confirm){
            break;
        }
    }
    // 将这几段话的分类输出到屏幕日志栏中
    QDebug logstr = qDebug();
    logstr << "GPT返回的每段内容的类别分别是：";
    for (int t = 0; t < paragraphs.size(); t++){
        logstr << QString::fromStdWString(get_opt_type_str(opt_type_list[t])) << "，";
    }
    return opt_type_list;
}

std::tuple<std::wstring, Opt, std::wstring> AnswerParser::parse_download_opt(std::wstring paragraph)
{
    // 先获取要下载的内容
    std::wstring package_name;
    int pos_start = paragraph.find(L"下载") + 2;
    while (pos_start < paragraph.size() && paragraph[pos_start] == L' '){
        pos_start += 1;
    }
    bool is_eng_package_name = is_eng_number(paragraph[pos_start]);
    for (int pos = pos_start; pos < paragraph.size(); pos++){
        if (is_eng_package_name){
            if (is_eng_number(paragraph[pos])){
                package_name += paragraph[pos];
            }else{
                break;
            }
        }else{
            if (std::find(Puncs.begin(), Puncs.end(), paragraph[pos]) == Puncs.end()){
                package_name += paragraph[pos];
            }else{
                break;
            }
        }
    }
    Opt empty_opt(0, L"", L"");
    std::wstring next_question = L"如何在";
    next_question.append(env_os);
    next_question.append(L"环境下，使用wget下载");
    next_question.append(package_name);
    next_question.append(L"（wget已安装）");
    // 获取要下载的路径
    std::wstring site;
    int pos_site = paragraph.find(L"http://");
    if (pos_site == std::wstring::npos){
        pos_site = paragraph.find(L"https://");
    }
    if (pos_site == std::wstring::npos){
        return std::tuple<std::wstring, Opt, std::wstring>{package_name, empty_opt, next_question};
    }
    wchar_t url_chars[] = L":-.?,'/\\+&%$#@~!=";
    int pos_site_end = -1;
    for (int t = pos_site; t != paragraph.size(); t++){
        // 判断这个字符是否为英文字符或特殊url字符，以此来判断url地址在段落中的位置。
        if (is_eng_number(paragraph[t])){
            continue;
        }
        bool is_special_url_char = false;
        for (int t0 = 0; t0 < wcslen(url_chars); t0++){
            if (paragraph[t] == url_chars[t0]){
                is_special_url_char = true;
                break;
            }
        }
        if (is_special_url_char){
            continue;
        }
        pos_site_end = t - 1;
        break;
    }
    site = paragraph.substr(pos_site, pos_site_end - pos_site + 1);
    // 根据下载的url地址，确定输出路径
    if (site.find(L"github.com") != std::wstring::npos){
        // 如果地址是github，则git clone
        std::wstring command = L"git clone ";
        std::wstring cp_site = site;
        command.append(cp_site);
        Opt opt(Download, L"", command);
        return std::tuple<std::wstring, Opt, std::wstring>{package_name, opt, L""};
    }else{
        // 如果地址不是github，则检查地址的后缀名，如果是一个压缩包，则wget
        std::vector<std::wstring> suffixs = {L".zip", L".rar", L".tar.gz", L".tar", L".7z", L".iso", L".exe", L".dll", L".lib", L".deb", L".a", L".so"};
        bool can_wget = false;
        for (auto it = suffixs.cbegin(); it != suffixs.cend(); it++){
            if (boost::iends_with(site, *it)){
                can_wget = true;
                break;
            }
        }
        if (can_wget){
            std::wstring command = L"wget ";
            command.append(site);
            Opt opt(Download, L"", command);
            return std::tuple<std::wstring, Opt, std::wstring>{package_name, opt, L""};
        }else{
            return std::tuple<std::wstring, Opt, std::wstring>{package_name, empty_opt, next_question};
        }
    }
}

std::wstring AnswerParser::remove_example(std::wstring paragraph)
{
    // 删除一段话中“例如”、“如”后面的内容。但如果遇到了中文分号或中文句号，再往后的内容可予以保留
    int start_dx = 0;
    bool del_status = false;  //当前内容是否删去的标志位
    std::wstring output_str;
    for (int t = 0; t < paragraph.size(); t++){
        if (del_status == false){
            if (t + 2 < paragraph.size() &&
                std::find(Puncs.begin(), Puncs.end(), paragraph[t]) != Puncs.end() &&
                paragraph[t + 1] == L'例' &&
                paragraph[t + 2] == L'如'){
                del_status = true;
                output_str.append(paragraph.substr(start_dx, t - start_dx));
            }
            if (t + 2 < paragraph.size() &&
                std::find(Puncs.begin(), Puncs.end(), paragraph[t]) != Puncs.end() &&
                paragraph[t + 1] == L'如' &&
                std::find(Puncs.begin(), Puncs.end(), paragraph[t + 2]) != Puncs.end()){
                del_status = true;
                output_str.append(paragraph.substr(start_dx, t - start_dx));
            }
        }else{
            if (paragraph[t] == L'。' || paragraph[t] == L'；'){
                del_status = false;
                start_dx = t;
            }
        }
    }

    if (!del_status){
        output_str.append(paragraph.substr(start_dx, paragraph.size() - start_dx));
    }
    return output_str;
}

bool AnswerParser::is_installed(std::wstring package_name)
{
    // 用cmd命令执行 where xxx，根据返回值判断软件是否已经安装
    std::wstring command = L"where ";
    command.append(package_name);
    ExecRes res = cmd_exec->exec_simple_command(command);
    if (res.return_value != 0){
        return false;
    }
    // 特例：如果where返回结果中，第一行内容包含了WindowsApps，则也应认定为未安装
    std::wstring cmdstr(res.out_str);
    std::wstring first_line;
    if (cmdstr.find(L"\n") != std::wstring::npos){
        first_line = cmdstr.substr(0, cmdstr.find(L"\n"));
    }else{
        first_line = cmdstr;
    }
    if (first_line.find(L"WindowsApps") != std::wstring::npos){
        return false;
    }
    return true;
}

int AnswerParser::is_confirm_install(std::wstring package_name)
{
    // 一个包是否已被确认过安装。0表示未被确认过，1表示被确认过没安装，2表示被确认过已安装
    auto find_res = std::find_if(confirm_install_res.begin(), confirm_install_res.end(), [&package_name](ConfirmInstallRes x){
        return bool(_wcsicmp(package_name.c_str(), x.package_name.c_str()) == 0);
    });
    if (find_res == confirm_install_res.end()){
        return 0;
    }
    bool is_installed = find_res->path.has_value();
    if (is_installed){
        return 2;
    }else{
        return 1;
    }
}

InstallList AnswerParser::get_package_list(std::wstring paragraph)
{
    // 根据GPT返回的内容，确定待安装的包列表
    InstallList pack_list;
    std::wstring package_name;
    int pos_start = paragraph.find(L"安装") + 2;
    while (pos_start < paragraph.size() && paragraph[pos_start] == L' '){
        pos_start += 1;
    }
    bool is_eng_package_name = is_eng_number(paragraph[pos_start]);
    bool is_next_package = true;
    for (int pos = pos_start; pos < paragraph.size(); pos++){
        if (is_eng_package_name){
            if (is_eng_number(paragraph[pos])){
                if (is_next_package){
                    package_name += paragraph[pos];
                }
            }else{
                if (package_name.size() != 0){
                    pack_list.push_back(package_name);
                    package_name.clear();
                    is_next_package = false;
                }
                if (paragraph[pos] == L'和' ||
                    paragraph[pos] == L'、' ||
                    (pos <= paragraph.size() - 2 && paragraph[pos] == L'以' && paragraph[pos] == L'及')){
                    is_next_package = true;
                }
                if (std::find(Puncs.begin(), Puncs.end(), paragraph[pos]) != Puncs.end()){
                    break;
                }
            }
        }else{
            if (std::find(Puncs.begin(), Puncs.end(), paragraph[pos]) == Puncs.end()){
                package_name += paragraph[pos];
            }else{
                pack_list = {package_name};
                package_name.clear();
                break;
            }
        }
    }
    if (package_name.size() != 0){
        pack_list.push_back(package_name);
    }
    return pack_list;
}

ReplaceList AnswerParser::check_replace(std::wstring raw_command, std::wstring dscp)
{
    ReplaceList replace_res;
    // 1.将解释段落按标点符号拆分
    std::vector<std::wstring> dscp_sentences;
    std::wstring sentence;
    for (int t = 0; t < dscp.size(); t++){
        if (std::find(Puncs.begin(), Puncs.end(), dscp[t]) != Puncs.end()){
            if (!sentence.empty()){
                dscp_sentences.push_back(sentence);
                sentence.clear();
            }
        }else{
            sentence.push_back(dscp[t]);
        }
    }
    if (!sentence.empty()){
        dscp_sentences.push_back(sentence);
    }
    // 2.检查是否有需要替换的命令。如果有需要替换的命令，则将替换的标的和值记录下来
    std::vector<std::wstring> replace_strs = {L"改为", L"改成", L"替换为", L"替换", L"是"};
    for (auto it = dscp_sentences.begin(); it != dscp_sentences.end(); it++){
        for (auto it0 = replace_strs.begin(); it0 != replace_strs.end(); it0++){
            int dx = it->find(*it0);
            if (dx != std::wstring::npos){
                std::wstring left;
                std::wstring right;
                int left_st_dx = dx;
                for (int t1 = dx - 1; t1 >= 0; t1--){
                    if (is_1byte_word((*it)[t1]) || std::find(ChnQuotations.begin(), ChnQuotations.end(), (*it)[t1]) != ChnQuotations.end()){
                        left_st_dx = t1;
                    }else{
                        break;
                    }
                }
                if (left_st_dx != dx){
                    left = it->substr(left_st_dx, dx - left_st_dx);
                }
                int right_ed_dx = dx + it0->size();
                for (int t1 = dx + it0->size(); t1 < it->size(); t1++){
                    if (std::find(Puncs.begin(), Puncs.end(), (*it)[t1]) == Puncs.end()){
                        right_ed_dx = t1;
                    }else{
                        break;
                    }
                }
                if (right_ed_dx != dx + it0->size()){
                    right = it->substr(dx + it0->size(), right_ed_dx - dx - it0->size() + 1);
                }
                left = trim_n(left);
                right = trim_n(right);
                if ((!left.empty()) && (!right.empty())){
                    std::wstring left2 = find_in_command(raw_command, left);
                    if (!left2.empty()){
                        std::tuple<std::wstring, std::wstring, std::wstring> replace{left2, *it0, right};
                        replace_res.push_back(replace);
                        break;
                    }
                }
            }
        }
    }
    return replace_res;
}

std::wstring AnswerParser::replace_command(std::wstring raw_command, ConfirmReplaceRes replace_res)
{
    // 根据replace列表，替换所有需要替换的内容。比如原始指令是 cd /<your_path>/x，replace_res里标出了<your_path>是什么，则应当将命令中的<your_path>替换成replace_res中的值
    std::wstring command = raw_command;
    for (int t = 0; t < replace_res.names.size(); t++){
        std::wstring target = std::get<0>(replace_res.names[t]);
        std::wstring value = replace_res.values[t];
        boost::replace_all(command, target, value);
    }
    return command;
}

int AnswerParser::find_not_check_ed_dx(std::vector<Section> sections)
{
    if (sections.empty()){
        return 0;
    }
    int not_check_ed_dx = sections.size();
    for (int t = sections.size() - 1; t >= 0; t--){
        if (sections[t].is_code == true && t != 0 && sections[t - 1].is_code == false){
            bool is_check = false;
            std::vector<std::wstring> verify_keywords = {L"验证", L"校验", L"检查", L"验证", L"测试", L"等待"};
            for (auto it = verify_keywords.begin(); it != verify_keywords.end(); it++){
                if (sections[t - 1].content.find(*it) != std::wstring::npos && sections[t - 1].content.find(*it) <= 10){
                    not_check_ed_dx = t - 1;
                    is_check = true;
                    break;
                }
            }
            if (!is_check){
                break;
            }
        }
    }
    return not_check_ed_dx;
}

std::tuple<bool, std::vector<Opt>, NeedConfirm> AnswerParser::parse_install_opt(std::wstring paragraph2, ConfirmReplaceRes replace_res)
{
    std::wstring paragraph = remove_example(paragraph2);
    // 1.先确认要安装的内容
    InstallList pack_list = get_package_list(paragraph);
    std::vector<bool> is_installed_list(pack_list.size(), false);
    Opt empty_opt(0, L"", L"");
    // 2.如果GPT提示要安装的包不是最终的安装结果，则检查待安装的内容是否已经安装了。如果已经安装了，则不需要重复安装。如果没有安装，则确认一下
    InstallList need_confirm_list;
    for (int t = 0; t < pack_list.size(); t++){
        std::wstring pack_name = pack_list[t];
        if (install_package.has_value() && _wcsicmp(pack_name.c_str(), install_package.value().c_str()) != 0){
            if (std::find_if(packages_to_download.begin(), packages_to_download.end(), [&pack_name](std::wstring x){
                return bool(_wcsicmp(pack_name.c_str(), x.c_str()) == 0);
            }) == packages_to_download.end()){
                int confirm_res = is_confirm_install(pack_name);
                if (is_installed(pack_name) || confirm_res == 2){
                    // 已经安装过，或已被确认安装过，则不需要重复安装
                    is_installed_list[t] = true;
                }else if (confirm_res == 0){
                    // 未被安装过，且未被确认安装过安装，则应当确认是否已安装
                    need_confirm_list.push_back(pack_name);
                }else{
                    // 未被安装过，且已被确认过没安装，则应该安装
                }
            }
        }
    }

    std::function<bool(std::vector<bool>)> all = [](std::vector<bool> v){
        for (int t = 0; t < v.size(); t++){
            if (v[t] == false){
                return false;
            }
        }
        return true;
    };
    if (all(is_installed_list) || need_confirm_list.size() != 0){
        // 如果已经全部安装过了，或者有仍需确认安装的内容，则返回，不执行后面的安装指令
        return std::tuple<bool, std::vector<Opt>, NeedConfirm>{true, {empty_opt}, need_confirm_list};
    }
    // 3.如果GPT提示要安装的包就是最终的安装结果，或者是下载的内容，则解析安装命令。如果没法解析出安装命令，则返回错误
    if (paragraph.find(L"```") == std::wstring::npos){
        return std::tuple<bool, std::vector<Opt>, NeedConfirm>{false, {empty_opt}, need_confirm_list};
    }
    std::vector<Section> sections = split_sections(paragraph);
    int not_check_ed_dx = find_not_check_ed_dx(sections);
    // 4.检查是否存在需要确认替换的内容
    std::vector<Opt> opt_list;
    ReplaceList confirm_replace_list;
    for (int t = 0; t < not_check_ed_dx; t++){
        if (sections[t].is_code == true){
            // 进入这里，说明这段内容是一段代码
            // 先解析这段命令是否需要在powershell中执行。如果是的话，调整命令内容
            bool is_powershell = false;
            if (t != 0 && sections[t - 1].is_code == false && boost::to_lower_copy(sections[t - 1].content).find(L"powershell") != std::wstring::npos){
                is_powershell = true;
            }
            if (boost::istarts_with(sections[t].code_mark, L"powershell")){
                is_powershell = true;
            }
            std::wstring command;
            if (is_powershell){
                command = convert_to_powershell(sections[t].content);
            }else{
                command = sections[t].content;
            }
            // 再检查这段命令是直接执行，还是需要进行替换确认
            ReplaceList confirm_replace;
            if (t < sections.size() - 1 && sections[t + 1].is_code == false){
                confirm_replace = check_replace(command, sections[t + 1].content);
            }
            if (confirm_replace.empty()){
                // 没有需要替换的内容，说明已经得到最终的命令了
                Opt opt(Install, L"", command);
                opt_list.push_back(opt);
            }else{
                // 有需要替换的内容，需要进一步确认如何替换。但如果已经确认过了就可以直接返回
                if (!replace_res.names.empty()){
                    std::wstring command2 = replace_command(command, replace_res);
                    Opt opt(Install, L"", command2);
                    opt_list.push_back(opt);
                }else{
                    confirm_replace_list.insert(confirm_replace_list.end(), confirm_replace.begin(), confirm_replace.end());
                }
            }
        }
    }
    return std::tuple<bool, std::vector<Opt>, ReplaceList>{true, opt_list, confirm_replace_list};
}

std::vector<AnswerParser::Section> AnswerParser::split_sections(std::wstring paragraph)
{
    // 将GPT返回的一条指令，进一步拆分成若干个小区段
    // 例如：paragraph是 创建文件，输入\n```rm -fr /```，就应当拆分成 {<创建文件，不是代码>，<rm -fr /，是代码>}

    std::vector<std::wstring> raw_sections;
    std::vector<Section> sections;
    boost::split_regex(raw_sections, paragraph, boost::wregex(L"\n"));
    bool is_code = false;
    std::wstring code_mark;
    std::wstring code_str;
    for (int t = 0; t < raw_sections.size(); t++){
        // 不处理空行
        if (raw_sections[t].empty()){
            continue;
        }
        // 判断当前区段是否在代码段中
        if (is_code){
            // 如果当前区段在代码段中
            std::wstring section_str = trim_n(raw_sections[t]);
            if (boost::ends_with(section_str, L"```")){
                int end_dx = section_str.rfind(L"```");
                if (end_dx != 0){
                    code_str.append(L"\n");
                    code_str.append(section_str.substr(0, end_dx));
                }
                code_str = trim_n(code_str);
                sections.push_back({code_str, true, code_mark});
                is_code = false;
                code_str.clear();
            }else{
                code_str.append(L"\n");
                code_str.append(raw_sections[t]);
            }
        }else{
            // 如果当前区段不在代码段中
            std::wstring section_str = trim_n(raw_sections[t]);
            bool is_code_begin = boost::starts_with(section_str, L"```");
            bool is_code_end = boost::ends_with(section_str, L"```");
            if (is_code_begin && is_code_end && section_str.size() >= 7){
                std::wstring code_str2 = section_str.substr(3, section_str.size() - 6);
                code_str2 = trim_n(code_str2);
                sections.push_back({code_str2, true, L""});
            }else if (is_code_begin){
                int st_dx = section_str.find(L"```") + 3;
                code_mark = section_str.substr(st_dx, section_str.size() - st_dx);
                is_code = true;
            }else{
                sections.push_back({raw_sections[t], false, L""});
            }
        }
    }
    return sections;
}

std::tuple<std::wstring, std::wstring> AnswerParser::get_exec_path(std::wstring section)
{
    // 是否有“文件”一词，如果有文件一词，则说明是在文件里执行，否则说明是在命令行中执行
    int file_dx = section.find(L"文件");
    if (file_dx == std::wstring::npos){
        return std::tuple<std::wstring, std::wstring>{L"", L""};
    }
    // 如果有文件一词，且在该词之后能够解析出一个完整的路径，说明是在某一个绝对路径下的文件中执行的
    std::wstring filename;
    bool in_path = false;
    for (int t = file_dx + 2; t < section.size(); t++){
        if (in_path == false){
            if (is_1byte_word(section[t]) && section[t] != L' ' && section[t] != L'\n'){
                filename.push_back(section[t]);
                in_path = true;
            }
        }else{
            if (is_1byte_word(section[t]) && section[t] != L'\n'){
                filename.push_back(section[t]);
            }else{
                in_path = false;
                break;
            }
        }
    }
    filename = remove_mark(trim_n(filename));
    if (filename.empty()){
        for (int t = file_dx - 1; t >= 0; t--){
            if (in_path == false){
                if (is_1byte_word(section[t]) && section[t] != L' ' && section[t] != L'\n'){
                    filename.insert(filename.begin(), section[t]);
                    in_path = true;
                }
            }else{
                if (is_1byte_word(section[t]) && section[t] != L'\n'){
                    filename.insert(filename.begin(), section[t]);
                }else{
                    in_path = false;
                    break;
                }
            }
        }
    }
    filename = remove_mark(trim_n(filename));
    // 如果能解析出完整的路径，需要检查这个路径是否为“合理”（比如/usr/lib/xxx或C:\XXX这类的内容）。如果不合理，也不能认定为一个路径
    if (!filename.empty()){
        if ((filename[0] == L'/' && (is_eng_number(filename[1]) || filename[1] == L'_')) ||
            (is_eng_chr(filename[0]) && filename[1] == L':' && filename[2] == L'\\')){
            return std::tuple<std::wstring, std::wstring>{filename, L""};
        }
    }
    // 如果有“文件”一词，但路径不明确，则需要返回确认信息，让用户确认文件路径是什么
    // std::vector<std::wstring> open_strs = {L"创建", L"新建", L"打开"};
    int st_dx = 0;
    int ed_dx = section.size() - 1;
    for (int t = file_dx; t >= 0; t--){
        if (std::find(Puncs.begin(), Puncs.end(), section[t]) != Puncs.end()){
            st_dx = t + 1;
            break;
        }
    }
    for (int t = file_dx + 2; t < section.size(); t++){
        if (std::find(Puncs.begin(), Puncs.end(), section[t]) != Puncs.end()){
            ed_dx = t - 1;
            break;
        }
    }
    std::wstring sentence = section.substr(st_dx, ed_dx - st_dx + 1);
    // for (auto it = open_strs.begin(); it != open_strs.end(); it++){
    if (filename.empty()){
        return std::tuple<std::wstring, std::wstring>{L"", sentence};
    }else{
        return std::tuple<std::wstring, std::wstring>{filename, sentence};
    }
    // }
    // return std::tuple<std::wstring, std::wstring>{L"", L""};
}

std::wstring AnswerParser::replace_path(std::wstring confirm_msg, ConfirmPathRes path_res)
{
    for (int t = 0; t < path_res.names.size(); t++){
        if (wcscmp(confirm_msg.c_str(), path_res.names[t].second.c_str()) == 0){
            return path_res.values[t];
        }
    }
    return L"";
}

std::wstring AnswerParser::convert_to_powershell(std::wstring raw_command)
{
    // 对于需要转换成powershell的形式的指令，进行处理
    // 注：只处理1行，且不包含powershell关键词的命令
    std::wstring command = raw_command;
    if (raw_command.find(L'\n') == std::wstring::npos && boost::to_lower_copy(raw_command).find(L"powershell") == std::wstring::npos){
        std::wstring tmp_command = raw_command;
        boost::replace_all(tmp_command, L"\"", L"\\\"");
        command = L"powershell -Command \"";
        command.append(tmp_command);
        command.append(L"\"");
    }
    return command;
}

std::tuple<std::vector<Opt>, PathList, ReplaceList> AnswerParser::parse_exec_opt(std::wstring paragraph2, ConfirmPathRes path_res, ConfirmReplaceRes replace_res)
{
    std::wstring paragraph = remove_example(paragraph2);

    std::vector<Opt> opt_list;
    PathList confirm_path_list;
    ReplaceList confirm_replace_list;

    // 1.解析出需要执行的命令，以及命令对应的上下文
    std::vector<Section> sections = split_sections(paragraph);
    int not_check_ed_dx = find_not_check_ed_dx(sections);

    for (int t = 0; t < not_check_ed_dx; t++){
        if (sections[t].is_code == true){
            // 进入这里，说明这段内容是一段代码
            // 2.解析这个命令应该在哪里执行，是打开一个文件还是在命令行
            std::wstring exec_pth = L"";
            std::wstring exec_pth_confirm = L"";
            if (t != 0 && sections[t - 1].is_code == false){
                std::tuple<std::wstring, std::wstring> exec_path = get_exec_path(sections[t - 1].content);
                exec_pth = std::get<0>(exec_path);
                exec_pth_confirm = std::get<1>(exec_path);
            }
            // 3.解析这个命令中有没有需要替换的内容。
            ReplaceList confirm_replace;
            if (t < sections.size() - 1 && sections[t + 1].is_code == false){
                confirm_replace = check_replace(sections[t].content, sections[t + 1].content);
            }
            // 4.解析这段命令是否需要在powershell中执行。如果是的话，调整命令内容
            bool is_powershell = false;
            if (t != 0 && sections[t - 1].is_code == false && boost::to_lower_copy(sections[t - 1].content).find(L"powershell") != std::wstring::npos){
                is_powershell = true;
            }
            if (boost::istarts_with(sections[t].code_mark, L"powershell")){
                is_powershell = true;
            }
            std::wstring command;
            if (is_powershell){
                command = convert_to_powershell(sections[t].content);
            }else{
                command = sections[t].content;
            }
            // 5.通过解析结果，分析这条命令可以直接执行，还是需要进一步确认
            if (exec_pth_confirm.empty() && confirm_replace.empty()){
                // 没有需要进一步的内容，可以直接执行
                Opt opt(Exec, exec_pth, command);
                opt_list.push_back(opt);
            }else{
                bool path_need_confirm = false;
                if (!exec_pth_confirm.empty()){
                    if (!path_res.names.empty()){
                        exec_pth = replace_path(exec_pth_confirm, path_res);
                    }else{
                        confirm_path_list.push_back(std::make_pair(exec_pth, exec_pth_confirm));
                        path_need_confirm = true;
                    }
                }
                if ((!confirm_replace.empty()) && (replace_res.names.empty())){
                    confirm_replace_list.insert(confirm_replace_list.end(), confirm_replace.begin(), confirm_replace.end());
                }else if (!path_need_confirm){
                    std::wstring command2;
                    if (replace_res.names.empty()){
                        command2 = command;
                    }else{
                        command2 = replace_command(command, replace_res);
                    }
                    if (boost::starts_with(exec_pth, L"cd:")){
                        std::wstring cdname = exec_pth.substr(3, exec_pth.size() - 3);
                        std::wstring cd_cmd = L"cd /d ";
                        cd_cmd.append(cdname);
                        Opt opt1(Exec, L"", cd_cmd);
                        Opt opt2(Exec, L"", command2);
                        opt_list.push_back(opt1);
                        opt_list.push_back(opt2);
                    }else{
                        Opt opt(Exec, exec_pth, command2);
                        opt_list.push_back(opt);
                    }
                }
            }
        }
    }
    return std::tuple<std::vector<Opt>, PathList, ReplaceList>{opt_list, confirm_path_list, confirm_replace_list};
}

std::wstring AnswerParser::del_special_char(std::wstring command_input)
{
    std::wstring command;
    std::vector<std::wstring> lines;
    boost::split(lines, command_input, boost::is_any_of(L"\n"));

    std::function<void(int)> add_n = [&command, &lines](int line_dx){
        if (line_dx < lines.size() - 1){
            command.append(L"\n");
        }
    };

    for (int line_it = 0; line_it < lines.size(); line_it++){
        std::wstring command_1line = trim_n(lines[line_it]);
        if (command_1line.empty()){
            add_n(line_it);
            continue;
        }
        // 去除 > 等符号。
        std::vector<std::wstring> tab_keywords = {L">", L"$", L"#"};
        int tab_find = false;
        for (auto it0 = tab_keywords.begin(); it0 != tab_keywords.end(); it0++){
            std::wstring keyword = (*it0);
            if (boost::starts_with(command_1line, *it0)){
                int st_dx = 0;
                for (int t = 0; t < command_1line.size(); t++){
                    if (keyword.find(command_1line[t]) == std::wstring::npos && command_1line[t] != L' '){
                        st_dx = t;
                        break;
                    }
                }
                command.append(command_1line.substr(st_dx, command_1line.size() - st_dx));
                add_n(line_it);
                tab_find = true;
                break;
            }
        }
        if (tab_find){
            continue;
        }
        // 判断这一行是否为注释，判断方法就是以 //为起始，且有中文。如果为注释，则这一行不用执行
        std::vector<std::wstring> comment_keywords = {L"//"};
        int comment_find = false;
        for (auto it0 = comment_keywords.begin(); it0 != comment_keywords.end(); it0++){
            if (boost::starts_with(command_1line, *it0) && has_chn_chr(command_1line)){
                comment_find = true;
                break;
            }
        }
        if (comment_find){
            continue;
        }
        // 如果这一行既没有 > 等符号，也判断为不是注释，则直接将这一行插入进去
        command.append(command_1line);
        add_n(line_it);
    }
    return command;
}

std::vector<Opt> AnswerParser::opt_process(std::vector<Opt> raw_opts)
{
    std::vector<Opt> opt_list;
    for (auto opt_it = raw_opts.begin(); opt_it != raw_opts.end(); opt_it++){
        Opt opt_new = (*opt_it);
        if (opt_new.file_path.empty()){
            opt_new.command = del_special_char(opt_new.command);
        }
        if (trim_n(opt_new.command).empty()){
            continue;
        }
        // 将github替换为国内源，github.com改为kgithub.com
        // 并且 git clone 或 wget 下载的包需要指定目标路径。下载路径为 download/包名
        if (boost::starts_with(opt_new.command, L"git clone") || boost::starts_with(opt_new.command, L"wget ")){
            std::wstring package_name = get_last_word(opt_new.command); //获取包名
//            std::wstring dest_folder = L"download/";
//            dest_folder.append(package_name);
            boost::replace_all(opt_new.command, L"github.com", L"kgithub.com");
//            if (boost::starts_with(opt_new.command, L"git clone")){
//                // git clone xxx指定下载路径的方法是 git clone https://github.com/xxxxx <目标路径>
//                opt_new.command.append(L" ");
//                opt_new.command.append(dest_folder);
//            }else{
//                // wget指定下载路径的方法是 wget https://xxx.com/xxxxx -P <目标路径>
//                opt_new.command.append(L" -P ");
//                opt_new.command.append(dest_folder);
//            }
            opt_list.push_back(opt_new);
            if (opt_new.opt_type == Download && _wcsicmp(package_name.c_str(), L"vcpkg") == 0){
                Opt opt_new2(Exec, L"", L"sed -i \"s/github.com/kgithub.com/g\" `grep -rl \"github.com\" ./vcpkg`");
                opt_list.push_back(opt_new2);
                Opt opt_new3(Exec, L"", L"sed -i \"s/raw.githubusercontent.com/raw.staticdn.net/g\" `grep -rl \"raw.githubusercontent.com\" ./vcpkg`");
                opt_list.push_back(opt_new3);
            }
            // 将下载后的路径添加进环境变量中
//            std::wstring opt_new4_command = L"set PATH=";
//            std::wstring abspath = boost::filesystem::absolute(dest_folder).wstring();
//            opt_new4_command.append(abspath);
//            opt_new4_command.append(L";%PATH%");
//            Opt opt_new4(Exec, L"", opt_new4_command);
//            opt_list.push_back(opt_new4);
            continue;
        }
        // 将vcpkg替换为国内源
        if (opt_new.opt_type == Download && _wcsicmp(get_last_word(opt_new.command).c_str(), L"vcpkg") == 0){
            opt_list.push_back(opt_new);
            Opt opt_new2(Exec, L"", L"sed -i \"s/github.com/kgithub.com/g\" `grep -rl \"github.com\" ./vcpkg`");
            opt_list.push_back(opt_new2);
            Opt opt_new3(Exec, L"", L"sed -i \"s/raw.githubusercontent.com/raw.staticdn.net/g\" `grep -rl \"raw.githubusercontent.com\" ./vcpkg`");
            opt_list.push_back(opt_new3);
            continue;
        }
        // 将pip install替换为国内（清华）源
        if (opt_new.opt_type == Install && opt_new.command.find(L"pip install ") != std::wstring::npos && opt_new.command.find(L" -i ") == std::wstring::npos){
            opt_new.command.append(L" -i https://pypi.tuna.tsinghua.edu.cn/simple");
            opt_list.push_back(opt_new);
            continue;
        }
        // 测试了一下，apt-get install或apt install使用现有源已经足够快了，不需要替换成国内源
        // 将npm install替换为国内源
        if (opt_new.opt_type == Install && opt_new.command.find(L"npm install ") != std::wstring::npos && opt_new.command.find(L" --registry=") == std::wstring::npos){
            opt_new.command.append(L" --registry=https://registry.npm.taobao.org/");
            opt_list.push_back(opt_new);
            continue;
        }
        // 对于cd 命令，如果后面的内容为一个盘符，则应该改成cd /d xxx。如 cd D:\Documents 应该改成 cd /d D:\Documents
        if (opt_new.command[0] == L'c' && opt_new.command[1] == L'd' && opt_new.command[2] == L' ' &&
            is_capital_eng_chr(opt_new.command[3]) && opt_new.command[4] == L':' && opt_new.command[5] == L'\\'){
            opt_new.command = L"cd /d ";
            opt_new.command.append(opt_new.command.substr(3, opt_new.command.size() - 3));
            opt_list.push_back(opt_new);
            continue;
        }
        // 其他情况，直接使用现有的命令
        opt_list.push_back(opt_new);
    }
    return opt_list;
}
