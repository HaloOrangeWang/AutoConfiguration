#pragma execution_character_set("utf-8")

#include "gpt_comm.h"
#include "constants.h"
#include "funcs.h"
#include <QDebug>
#include <rapidjson/document.h>

GptComm::GptComm()
{
    ws_client.clear_access_channels(websocketpp::log::alevel::all);
    ws_client.clear_error_channels(websocketpp::log::elevel::all);

    ws_client.init_asio();
    ws_client.start_perpetual();

    conn_thread.reset(new websocketpp::lib::thread(&Client::run, &ws_client));
}

void GptComm::input_question(std::wstring question_str)
{
    // 如果有一个问题正在处理，则不能提出新的问题
    if (is_busy){
        emit ReturnErrorMsg(Busy);
        return;
    }
    is_busy = true;
    // 缓存这个问题
    question = question_str;
    // 连接服务器
    int error_id = connect_server();
    if (error_id != Success){
        emit ReturnErrorMsg(error_id);
        is_busy = false;
        return;
    }
}

int GptComm::connect_server()
{
    qDebug() << "正在连接Websocket服务器";
    // 连接服务器
    websocketpp::lib::error_code ec;
    conn = ws_client.get_connection(ServerURL, ec);
    if (ec){
        return FailedToConnect;
    }
    // 绑定回调函数
    conn->set_open_handler(websocketpp::lib::bind(&GptComm::on_open, this, &ws_client, websocketpp::lib::placeholders::_1));
    conn->set_fail_handler(websocketpp::lib::bind(&GptComm::on_fail, this, &ws_client, websocketpp::lib::placeholders::_1));
    conn->set_message_handler(websocketpp::lib::bind(&GptComm::on_message, this, &ws_client, websocketpp::lib::placeholders::_1, websocketpp::lib::placeholders::_2));
    conn->set_close_handler(websocketpp::lib::bind(&GptComm::on_close, this, &ws_client, websocketpp::lib::placeholders::_1));
    ws_client.connect(conn);
    return Success;
}

void GptComm::on_open(Client* c, websocketpp::connection_hdl hdl)
{
    qDebug() << "Websocket服务器连接成功，正在发送问题";
    //连接成功后，将问题发送给GPT，等待收到答案
    std::string question2 = UnicodeToUTF8(question);
    ws_client.send(hdl, question2, websocketpp::frame::opcode::text);
}

void GptComm::send_next_question(std::wstring question_str)
{
    qDebug() << "程序向GPT追加提问";
    std::string question2 = UnicodeToUTF8(question_str);
    ws_client.send(conn->get_handle(), question2, websocketpp::frame::opcode::text);
}

void GptComm::on_fail(Client* c, websocketpp::connection_hdl hdl)
{
    qDebug() << "Websocket异常";
    // 连接时出错（比如404了），会调用这个函数
    std::string error_reason = c->get_con_from_hdl(hdl)->get_ec().message();
    emit ReturnErrorMsg(WebSocketError);
    try{
        c->close(hdl, websocketpp::close::status::normal, ""); //出错之后直接关闭连接即可。
    }catch(std::exception& ex){
        is_busy = false;
    }
}

void GptComm::emit_err_msg(int error_id, Client* c, websocketpp::connection_hdl& hdl)
{
    emit ReturnErrorMsg(error_id);
    c->close(hdl, websocketpp::close::status::normal, ""); //出错之后直接关闭连接即可。
}

void GptComm::on_close(Client* c, websocketpp::connection_hdl hdl)
{
    // 如果连接已经结束了，则将websocket客户端状态置为“闲置”，允许新的连接即可。
    is_busy = false;
}

void GptComm::on_message(Client* c, websocketpp::connection_hdl hdl, Client::message_ptr msg)
{
    qDebug() << "收到了Websocket返回的信息";
    // 1.先判断是否正确接收到答案了
    std::string answer;
    if (msg->get_opcode() == websocketpp::frame::opcode::text){
        std::string raw_msg = msg->get_payload();
        rapidjson::Document document;
        document.Parse(raw_msg.c_str());
        if (document.HasParseError() || (!document.HasMember("err_code")) || (!document["err_code"].IsNumber())){
            emit_err_msg(ServerError, c, hdl);
            return;
        }
        int error_code = document["err_code"].GetInt();
        if (error_code == 1){
            // error_code=1说明是server在和GPT通信时出现了问题
            emit_err_msg(GPTError, c, hdl);
            return;
        }else if(error_code != 0){
            // error_code=2说明是server内部出现了问题
            emit_err_msg(ServerError, c, hdl);
            return;
        }
        if ((!document.HasMember("answer")) || (!document["answer"].IsString())){
            emit_err_msg(ServerError, c, hdl);
            return;
        }
        answer = document["answer"].GetString();
    }else{
        emit_err_msg(WebSocketError, c, hdl);
        return;
    }

    // 2.分析问题的答案
    std::wstring answer_ws = UTF8ToUnicode(answer);
    emit ReturnAnswer(answer_ws);
}

GptComm::~GptComm()
{
    ws_client.stop_perpetual();
    try{
        websocketpp::lib::error_code ec;
        ws_client.close(ws_client.get_con_from_hdl(conn->get_handle()), websocketpp::close::status::going_away, "", ec);
    }catch(std::exception& ex){

    }
    conn_thread->join();
}
