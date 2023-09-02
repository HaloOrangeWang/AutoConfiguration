# coding=utf8

import asyncio
import tornado.websocket
import tornado.web
import json
from enum import IntEnum


class ErrorCode(IntEnum):
    Success = 0
    GPTError = 1  # 和GPT Server连接时出现异常
    MsgError = 2  # 原始消息异常


class MainServer(tornado.websocket.WebSocketHandler):

    # noinspection PyAttributeOutsideInit
    def initialize(self):
        self.question_id = 0

    def open(self):
        pass

    async def on_message(self, message):
        """处理客户端发来的消息。依次将消息解码、保存并发送给GPT"""
        # 1.先将消息解码
        with open("git_ssh/conf.txt", "r", encoding="utf8") as f:
            rtn_message = f.read()
        with open("git_ssh/check.txt", "r", encoding="utf8") as f:
            check_message = f.read()
        self.question_id += 1

        rtn_msg = json.dumps({"err_code": ErrorCode.Success, "origin_answer": rtn_message, "check_answer": check_message})
        await self.write_message(rtn_msg.encode("utf8"))

    def on_close(self):
        pass


async def main():
    # 初始化websocket server
    app = tornado.web.Application([
        (r"/submit", MainServer)
    ])
    app.listen(58000)
    await asyncio.Event().wait()


if __name__ == '__main__':
    asyncio.run(main())
