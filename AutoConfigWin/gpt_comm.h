#ifndef GPT_COMM_H
#define GPT_COMM_H

#include "answer_parser.h"
#include <QObject>
#include <string>
#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>

typedef websocketpp::client<websocketpp::config::asio_client> Client;

class GptComm: public QObject
{
    Q_OBJECT

public:
    void input_question(std::wstring question_str); //输入一个问题，它将自动完成与GPT交互的过程。
    void send_next_question(std::wstring question_str);  //向GPT服务器追加问题
    bool is_busy = false; //是否正在处理上一个问题

    enum ErrorID
    {
        Success = 0,
        Busy,
        FailedToConnect,
        WebSocketError,
        ServerError,
        GPTError
    };

signals:
    void ReturnErrorMsg(int error_id); //如果向GPT发送提问请求的过程出现异常，则返回这个信号
    void ReturnAnswer(std::wstring answer2); //返回答案后通过这个函数通知主线程

private:
    int connect_server(); //连接websocket服务器
    // websocket连接成功/失败/收到消息等的回调
    void on_open(Client* c, websocketpp::connection_hdl hdl);
    void on_fail(Client* c, websocketpp::connection_hdl hdl);
    void on_message(Client* c, websocketpp::connection_hdl hdl, Client::message_ptr msg);
    void on_close(Client* c, websocketpp::connection_hdl hdl);
    // 向MainWindow发送错误标记，并执行后续处理
    void emit_err_msg(int error_id, Client* c, websocketpp::connection_hdl& hdl);

private:
    std::wstring question; // 需要询问的问题内容
    // websocket的客户端、连接
    Client ws_client;
    websocketpp::lib::shared_ptr<websocketpp::lib::thread> conn_thread;
    Client::connection_ptr conn;

public:
    GptComm();    
    ~GptComm();
};

#endif // GPT_COMM_H
