from models import SqlModel
from gpt_client import GptClient
import asyncio
import tornado.websocket
import tornado.web
import json
from enum import IntEnum
from typing import Optional


class ErrorCode(IntEnum):
    Success = 0
    GPTError = 1  # 和GPT Server连接时出现异常
    MsgError = 2  # 原始消息异常


class MainServer(tornado.websocket.WebSocketHandler):

    # noinspection PyAttributeOutsideInit
    def initialize(self, sql_model: SqlModel, gpt_client: GptClient):
        self.sql_model = sql_model
        self.gpt_client = gpt_client

    def open(self):
        pass

    def return_answer_and_errcode(self, err_code: ErrorCode, answer: Optional[str] = None):
        if err_code != ErrorCode.Success:
            rtn_msg = json.dumps({"err_code": err_code})
        else:
            rtn_msg = json.dumps({"err_code": err_code, "answer": answer})
        self.write_message(rtn_msg.encode("utf8"))

    async def on_message(self, message):
        """处理客户端发来的消息。依次将消息解码、保存并发送给GPT"""
        # 1.现将消息解码
        try:
            if type(message) is bytes:
                message = message.decode("utf8")
        except UnicodeDecodeError:
            self.return_answer_and_errcode(ErrorCode.MsgError)
        # 2.保存消息
        self.sql_model.store_question(message)
        # 3.将消息发送给GPT，并等待答案
        answer = await asyncio.get_running_loop().run_in_executor(self.gpt_client.threadpool, self.gpt_client.submit_question, message)
        if answer is None:
            self.return_answer_and_errcode(ErrorCode.GPTError)
        else:
            self.return_answer_and_errcode(ErrorCode.Success, answer)

    def on_close(self):
        pass


async def main():
    # 初始化数据库和线程池
    sql_model = SqlModel()
    gpt_api_keys = sql_model.get_gpt_keys()
    assert len(gpt_api_keys) != 0
    gpt_client = GptClient(gpt_api_keys)
    # 初始化websocket server
    app = tornado.web.Application([
        (r"/submit", MainServer, dict(sql_model=sql_model, gpt_client=gpt_client))
    ])
    app.listen(58000, address="0.0.0.0")
    await asyncio.Event().wait()


if __name__ == '__main__':
    asyncio.run(main())
