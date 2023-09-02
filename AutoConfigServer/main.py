from models import SqlModel
from gpt_client import GptClient
from logs import Log
from question import format_origin_question, format_check_question
import asyncio
import tornado.websocket
import tornado.web
import json
from enum import IntEnum
from typing import Optional
import traceback
import os


def init_sensi_word():
    sensi_words2 = []
    if os.path.exists("sensi_words.txt"):
        sensi_words = open("sensi_words.txt", "r", encoding="utf-8").readlines()
        sensi_words = sensi_words[:-1]
        for t in range(len(sensi_words)):
            if sensi_words[t].endswith("\n"):
                sensi_words[t] = sensi_words[t][:-1]
        for word in sensi_words:
            if len(word) >= 2 and max([ord(t) for t in word]) >= 0x4000:
                sensi_words2.append(word)
    return sensi_words2


class ErrorCode(IntEnum):
    Success = 0
    GPTError = 1  # 和GPT Server连接时出现异常
    MsgError = 2  # 原始消息异常


# noinspection PyAbstractClass
class MainServer(tornado.websocket.WebSocketHandler):

    # noinspection PyAttributeOutsideInit
    def initialize(self, sql_model: SqlModel, gpt_client: GptClient, sensi_words):
        self.sql_model = sql_model
        self.gpt_client = gpt_client
        self.sensi_words = sensi_words
        # self.is_first_question = True  # 是不是该客户端提出的第一个问题
        self.question_id: Optional[int] = None  # 问题id

    def open(self):
        Log.print_log('有新连接')
        pass

    def return_answer_and_errcode(self, err_code: ErrorCode, origin_answer: Optional[str] = None, check_answer: Optional[str] = None):
        if err_code != ErrorCode.Success:
            rtn_msg = json.dumps({"err_code": err_code})
        else:
            rtn_msg = json.dumps({"err_code": err_code, "origin_answer": origin_answer, "check_answer": check_answer})
        self.write_message(rtn_msg.encode("utf8"))

    # noinspection PyAttributeOutsideInit
    async def on_message(self, message):
        """处理客户端发来的消息。依次将消息解码、保存并发送给GPT"""
        for word in self.sensi_words:
            if word in message:
                Log.print_log("sensi_words中的词 %s 在输入的句子中" % word)
                # print(word)
                # self.return_answer_and_errcode(ErrorCode.MsgError)
                # return
        # 1.先将消息解码。
        try:
            if type(message) is bytes:
                message = message.decode("utf8")
            question_dict = json.loads(message)
        except (UnicodeDecodeError, json.decoder.JSONDecodeError):
            Log.print_log("原始消息解码失败")
            self.return_answer_and_errcode(ErrorCode.MsgError)
            return
        # noinspection PyBroadException
        try:
            # 2.保存客户端提出的原始问题
            self.question_id = self.sql_model.store_first_question(message)
            # 注：0.92版本没有“客户端向服务端连续发问”的场景。

            # 3.生成第一个问题对应的prompt，将prompt发送给GPT，并等待答案
            prompt1 = format_origin_question(question_dict)
            answer1 = await asyncio.get_running_loop().run_in_executor(self.gpt_client.threadpool, self.gpt_client.submit_question, prompt1)
            if answer1 is None:
                self.return_answer_and_errcode(ErrorCode.GPTError)
            else:
                Log.print_log("消息返回成功")

            # 4.生成第二个问题（去除检查环节）对应的prompt，将prompt发送给GPT，并等待答案
            prompt2 = format_origin_question(question_dict)
            answer2 = await asyncio.get_running_loop().run_in_executor(self.gpt_client.threadpool, self.gpt_client.submit_question, prompt2)
            if answer2 is None:
                self.return_answer_and_errcode(ErrorCode.GPTError)
            else:
                Log.print_log("消息返回成功")
                self.return_answer_and_errcode(ErrorCode.Success, answer1, answer2)

            # 5.将回答保存到mongodb数据库中
            self.sql_model.store_next_question(self.question_id, prompt1)
            self.sql_model.store_next_question(self.question_id, prompt2)
            self.sql_model.update_first_answer(self.question_id, answer1)
            self.sql_model.update_next_answer(self.question_id, answer2)
        except Exception:
            Log.print_log("服务端内部异常： %s" % traceback.format_exc())
            self.return_answer_and_errcode(ErrorCode.MsgError)

    def on_close(self):
        pass


# noinspection PyAbstractClass
class ReportHandler(tornado.web.RequestHandler):

    # noinspection PyAttributeOutsideInit
    def initialize(self, sql_model: SqlModel):
        self.sql_model = sql_model

    def get(self):
        self.render("frontend/report.html")

    def post(self):
        feedback = self.get_argument("feedback")
        question = self.get_argument("question")
        answer = self.get_argument("answer")
        bugreport = self.get_argument("bugreport")
        self.sql_model.store_feedback(feedback, question, answer, bugreport)
        return self.render("frontend/report_success.html")


async def main():
    # 初始化日志、数据库和线程池
    Log.init()
    sql_model = SqlModel()
    gpt_api_keys = sql_model.get_gpt_keys()
    assert len(gpt_api_keys) != 0
    gpt_client = GptClient(gpt_api_keys)
    sensi_words = init_sensi_word()
    # 初始化websocket server
    app = tornado.web.Application([
        (r"/submit", MainServer, dict(sql_model=sql_model, gpt_client=gpt_client, sensi_words=sensi_words)),
        (r"/report", ReportHandler, dict(sql_model=sql_model)),
        (r"/static/(.*)", tornado.web.StaticFileHandler, {"path": "frontend/static"}),
    ])
    app.listen(58000, address="0.0.0.0")
    await asyncio.Event().wait()


if __name__ == '__main__':
    asyncio.run(main())
