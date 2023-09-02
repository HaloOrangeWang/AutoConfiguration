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
    cmd_exec = cmd_exec_;
}

bool AnswerParser::parse_first_answer(std::wstring origin_answer, std::wstring check_answer)
{
    // 1.先将原始答案分段
    std::vector<std::wstring> paragraphs = get_exec_paragraphs(origin_answer, check_answer);
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
        }else if (opt_type == Install){
            // 处理安装指令
            ConfirmReplaceRes confirm_res_curnode{};
            if (confirm_replace_res.find(curr_node) != confirm_replace_res.end()){
                confirm_res_curnode = confirm_replace_res.at(curr_node);
            }
            std::tuple<bool, std::vector<Opt>, ReplaceList> res = parse_install_opt(para_tree[curr_node].text, confirm_res_curnode);
            if (std::get<0>(res) == true){
                ReplaceList confirm = std::get<2>(res);
                if (confirm.empty()){
                    std::vector<Opt> opt_list = std::get<1>(res);
                    for (auto it = opt_list.begin(); it != opt_list.end(); it++){
                        parse_result.opt_list.push_back(*it);
                    }
                }else{
                    emit ConfirmReplace(confirm, para_tree[curr_node].text);
                    return;
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
                emit ParseSuccess(parse_result.opt_list);
                return;
            }
        }
    }
}

std::vector<std::wstring> AnswerParser::get_exec_paragraphs(std::wstring origin_answer, std::wstring check_answer)
{
    // 1.将答案按照'\n'拆分
    std::vector<std::wstring> raw_paragraphs;
    std::vector<std::wstring> paragraphs;
    boost::split_regex(raw_paragraphs, origin_answer, boost::wregex(L"\n"));
    // 2.按照命令id进行组合
    // 如果这行内容以 a. 开头（a为当前command_id），则说明它开启了一个新命令，否则说明它不是新命令的起始
    boost::wregex regstr(L"^(?<id2>[0-9]+)\\.(.*)");
    int command_id = 0;
    for (int para_it = 0; para_it < raw_paragraphs.size(); para_it++){
        boost::wsmatch match_res;
        if (boost::regex_match(raw_paragraphs[para_it], match_res, regstr)){
            int command_id2 = stoi(match_res.str(L"id2"));
            if (command_id2 == command_id + 1){
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
    // 3.根据check_answer中的内容，去除仅用于检查的段落
    std::vector<std::wstring> raw_check_paragraphs;
    std::vector<int> check_para_dxs; //标注哪些段落是仅仅用于校验的。
    boost::split_regex(raw_check_paragraphs, check_answer, boost::wregex(L"\n"));
    for (int t = raw_check_paragraphs.size() - 1; t >= 0; t--){
        int pos1 = raw_check_paragraphs[t].find(L"[");
        int pos2 = raw_check_paragraphs[t].find(L"]");
        if (pos1 != std::wstring::npos && pos2 != std::wstring::npos && pos1 < pos2){
            if (pos2 - pos1 == 1){
                // 如果给出的检查段落id列表是[]，则说明没有“仅用于检查”的段落。
                break;
            }
            std::wstring substr = raw_check_paragraphs[t].substr(pos1 + 1, pos2 - pos1 - 1);
            std::vector<std::wstring> raw_id_list;
            boost::split(raw_id_list, substr, boost::is_any_of(L","));
            for (int t0 = 0; t0 < raw_id_list.size(); t0++){
                // 这里减1是因为GPT返回的索引是从1开始计数的，而paragraph数组是从0开始计数的
                check_para_dxs.push_back(stoi(raw_id_list[t0]) - 1);
            }
            break;
        }
    }
    std::vector<std::wstring> exec_paragraphs;
    for (int t = 0; t < paragraphs.size(); t++){
        if (std::find(check_para_dxs.begin(), check_para_dxs.end(), t) == check_para_dxs.end()){
            exec_paragraphs.push_back(paragraphs[t]);
        }
    }
    return exec_paragraphs;
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
    // 将这几段话的分类输出到屏幕日志栏中
    QDebug logstr = qDebug();
    logstr << "GPT返回的每段内容的类别分别是：";
    for (int t = 0; t < paragraphs.size(); t++){
        logstr << QString::fromStdWString(get_opt_type_str(opt_type_list[t])) << "，";
    }
    return opt_type_list;
}

bool AnswerParser::is_installed(std::wstring command)
{
    ExecRes res = cmd_exec->exec_simple_command(command);
    if (res.return_value != 0){
        return false;
    }
    return true;
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

std::tuple<bool, std::vector<Opt>, ReplaceList> AnswerParser::parse_install_opt(std::wstring paragraph, ConfirmReplaceRes replace_res)
{
    std::vector<Opt> opt_list;
    ReplaceList confirm_replace_list;

    // 1.将GPT返回的内容拆分出区段
    std::vector<Section> sections = split_sections(paragraph);

    int installed_flag = Unconfirm;
    for (int section_it = 0; section_it < sections.size(); section_it++){
        // 2.如果GPT返回的内容是“检查XXX是否已安装”，那么提前执行这条指令
        std::vector<std::wstring> verify_keywords = {L"验证", L"校验", L"检查", L"测试"};
        bool is_confirm = false;
        for (auto it = verify_keywords.begin(); it != verify_keywords.end(); it++){
            if (sections[section_it].aim.find(*it) != std::wstring::npos){
                is_confirm = true;
                break;
            }
        }
        if (is_confirm){
            std::wstring command = handle_powershell(sections[section_it].content, sections[section_it].loc);
            if (is_installed(command)){
                installed_flag = Installed;
            }else{
                installed_flag = NotInstalled;
            }
        }else{
            // 3.如果确定是安装命令，且对应的包并未安装，则需要执行命令。
            if (installed_flag != Installed){
                // 如果这条指令的“说明”内容中有需要替换的内容，则需要确认替换，再执行。否则可以直接执行。
                ReplaceList confirm_replace;
                confirm_replace = check_replace(sections[section_it].content, sections[section_it].mark);
                if (confirm_replace.empty()){
                    // 没有需要替换的内容，说明已经得到最终的命令了
                    std::wstring command = handle_powershell(sections[section_it].content, sections[section_it].loc);
                    Opt opt(Install, L"", command);
                    opt_list.push_back(opt);
                }else{
                    // 有需要替换的内容，需要进一步确认如何替换。但如果已经确认过了就可以直接返回
                    if (!replace_res.names.empty()){
                        std::wstring raw_command = replace_command(sections[section_it].content, replace_res);
                        std::wstring command = handle_powershell(raw_command, sections[section_it].loc);
                        Opt opt(Install, L"", command);
                        opt_list.push_back(opt);
                    }else{
                        confirm_replace_list.insert(confirm_replace_list.end(), confirm_replace.begin(), confirm_replace.end());
                    }
                }
            }
            installed_flag = Unconfirm;
        }
    }
    return std::tuple<bool, std::vector<Opt>, ReplaceList>{true, opt_list, confirm_replace_list};
}

std::vector<AnswerParser::Section> AnswerParser::split_sections(std::wstring paragraph)
{
    // 将GPT返回的一条指令，进一步拆分成若干个小区段
    // 例如：命令的目的是验证python是否已安装，执行位置是cmd，命令内容是python --version

    std::vector<std::wstring> raw_sections;
    std::vector<Section> sections;
    boost::split_regex(raw_sections, paragraph, boost::wregex(L"\n"));
    bool is_code = false;
    Section curr_section;

    std::function<void()> add_useful_section = [&sections, &curr_section](){
        // 如果当前段落已经解析出命令内容了，则将其添加到最终的命令列表中
        if (!curr_section.content.empty()){
            if (sections.size() >= 1){
                int last_dx = sections.size() - 1;
                if (curr_section.aim.empty()){
                    curr_section.aim = sections[last_dx].aim;
                }
                if (curr_section.loc.empty()){
                    curr_section.loc = sections[last_dx].loc;
                }
                curr_section.mark = sections[last_dx].mark;
            }
            sections.push_back(curr_section);
            curr_section = Section();
        }
    };

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
                    curr_section.content.append(L"\n");
                    curr_section.content.append(section_str.substr(0, end_dx));
                }
                curr_section.content = trim_n(curr_section.content);
                is_code = false;
            }else{
                curr_section.content.append(L"\n");
                curr_section.content.append(raw_sections[t]);
            }
        }else{
            // 如果当前区段不在代码段中
            // 先检查这一区段是否存在代码
            std::wstring section_str = trim_n(raw_sections[t]);
            std::wstring section_sp_str = trim_n_spechar(raw_sections[t]);
            std::wstring first_sentence = get_first_sentence(section_sp_str);
            bool is_code_begin = boost::starts_with(section_str, L"```");
            bool is_code_end = boost::ends_with(section_str, L"```");
            if (is_code_begin && is_code_end && section_str.size() >= 7){
                add_useful_section();
                curr_section.content = section_str.substr(3, section_str.size() - 6);
                curr_section.content = trim_n(curr_section.content);
            }else if (is_code_begin){
                add_useful_section();
                int st_dx = section_str.find(L"```") + 3;
                curr_section.content = section_str.substr(st_dx, section_str.size() - st_dx);
                is_code = true;
            }else{
                // 再检查这一区段是否代表了代码的目的、地址、说明
                if (first_sentence.find(L"目的") != std::wstring::npos){
                    add_useful_section();
                    curr_section.aim = trim_n_spechar(section_sp_str);
                }else if (first_sentence.find(L"地址") != std::wstring::npos){
                    add_useful_section();
                    curr_section.loc = trim_n_spechar(section_sp_str);
                }else if (first_sentence.find(L"说明") != std::wstring::npos){
                    curr_section.mark = trim_n_spechar(section_sp_str);
                }
            }
        }
    }
    // 如果有尚未添加的区段，将其添加进去
    add_useful_section();

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

std::wstring AnswerParser::handle_powershell(std::wstring raw_command, std::wstring exec_path)
{
    bool need_powershell = false;
    bool has_powershell = false;
    if (boost::to_lower_copy(raw_command).find(L"powershell") != std::wstring::npos){
        has_powershell = true;
    }
    if (boost::to_lower_copy(exec_path).find(L"powershell") != std::wstring::npos){
        need_powershell = true;
    }
    std::wstring command;
    if (need_powershell && (!has_powershell)){
        command = convert_to_powershell(raw_command);
    }else{
        command = raw_command;
    }
    return command;
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

std::tuple<std::vector<Opt>, PathList, ReplaceList> AnswerParser::parse_exec_opt(std::wstring paragraph, ConfirmPathRes path_res, ConfirmReplaceRes replace_res)
{
    std::vector<Opt> opt_list;
    PathList confirm_path_list;
    ReplaceList confirm_replace_list;

    // 1.将GPT返回的内容拆分出区段
    std::vector<Section> sections = split_sections(paragraph);

    for (int section_it = 0; section_it < sections.size(); section_it++){
        // 2.解析这个命令应该在哪里执行，是打开一个文件，还是在命令行，还是powershell
        std::wstring exec_pth = L"";
        std::wstring exec_pth_confirm = L"";
        std::tuple<std::wstring, std::wstring> exec_path = get_exec_path(sections[section_it].loc);
        exec_pth = std::get<0>(exec_path);
        exec_pth_confirm = std::get<1>(exec_path);
        std::wstring raw_command = sections[section_it].content;
        if (exec_pth_confirm.empty()){
            // 没有需要进一步的内容，可以直接执行
        }else{
            // 如果有需要进一步确认的内容，则需要判断是否有需要进一步确认，还是直接替换即可
            if (!path_res.names.empty()){
                // 已经确认过这些要替换的内容了，不需要重新替换
                exec_pth = replace_path(exec_pth_confirm, path_res);
            }else{
                confirm_path_list.push_back(std::make_pair(exec_pth, exec_pth_confirm));
            }
        }
        // 3.检查命令内容中是否有需要替换的内容。如果这条指令的“说明”内容中有需要替换的内容，则需要确认替换，再执行。否则可以直接执行。
        ReplaceList confirm_replace;
        confirm_replace = check_replace(raw_command, sections[section_it].mark);
        if (confirm_replace.empty()){
            // 没有需要替换的内容，说明已经得到最终的命令了
        }else{
            // 有需要替换的内容，需要进一步确认如何替换。但如果已经确认过了就可以直接返回
            if (!replace_res.names.empty()){
                raw_command = replace_command(raw_command, replace_res);
            }else{
                confirm_replace_list.insert(confirm_replace_list.end(), confirm_replace.begin(), confirm_replace.end());
            }
        }
        // 4.正式插入命令。
        // 如果确定是写入到一个文件中，那么就不需要检查是不是“powershell”了，否则需要进一步检查是cmd还是powershell。
        std::wstring command;
        if (sections[section_it].loc.find(L"文件") == std::wstring::npos){
            command = handle_powershell(raw_command, sections[section_it].loc);
        }else{
            command = sections[section_it].content;
        }
        if (confirm_path_list.empty() && confirm_replace_list.empty()){
            if (boost::starts_with(exec_pth, L"cd:")){
                std::wstring cdname = exec_pth.substr(3, exec_pth.size() - 3);
                std::wstring cd_cmd = L"cd /d ";
                cd_cmd.append(cdname);
                Opt opt1(Exec, L"", cd_cmd);
                Opt opt2(Exec, L"", command);
                opt_list.push_back(opt1);
                opt_list.push_back(opt2);
            }else{
                Opt opt(Exec, exec_pth, command);
                opt_list.push_back(opt);
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
//            if (opt_new.opt_type == Download && _wcsicmp(package_name.c_str(), L"vcpkg") == 0){
//                Opt opt_new2(Exec, L"", L"sed -i \"s/github.com/kgithub.com/g\" `grep -rl \"github.com\" ./vcpkg`");
//                opt_list.push_back(opt_new2);
//                Opt opt_new3(Exec, L"", L"sed -i \"s/raw.githubusercontent.com/raw.staticdn.net/g\" `grep -rl \"raw.githubusercontent.com\" ./vcpkg`");
//                opt_list.push_back(opt_new3);
//            }
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
//        if (opt_new.opt_type == Download && _wcsicmp(get_last_word(opt_new.command).c_str(), L"vcpkg") == 0){
//            opt_list.push_back(opt_new);
//            Opt opt_new2(Exec, L"", L"sed -i \"s/github.com/kgithub.com/g\" `grep -rl \"github.com\" ./vcpkg`");
//            opt_list.push_back(opt_new2);
//            Opt opt_new3(Exec, L"", L"sed -i \"s/raw.githubusercontent.com/raw.staticdn.net/g\" `grep -rl \"raw.githubusercontent.com\" ./vcpkg`");
//            opt_list.push_back(opt_new3);
//            continue;
//        }
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
