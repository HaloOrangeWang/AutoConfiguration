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
        # 1.现将消息解码
        # rtn_message = "您可以按照以下步骤在 Windows 上使用命令行安装 ZeroMQ：\n\n1. 下载 ZeroMQ 的二进制文件，例如：https://download.opensuse.org/repositories/network:/messaging:/zeromq:/git-stable/Windows/\n\n2. 打开命令提示符或 PowerShell 并将当前目录更改为 ZeroMQ 二进制文件所在的目录。\n\n3. 运行以下命令，安装 ZeroMQ：\n\n```\nmsiexec /i zeromq-{version}-Win64-vc{vc_version}-libpgm-{pgm_version}.msi /quiet\n```\n其中，`{version}` 是 ZeroMQ 版本，在文件名中查找；`{vc_version}` 是 Visual Studio 版本，例如 2019 为 `14`，2017 为 `15`；`{pgm_version}` 是 PGM 协议库的版本，需要与 ZeroMQ 版本匹配。例如，如果您下载了 `zeromq-4.3.3-Win64-vc142-libpgm-5.3.0.msi`，则应该运行：\n\n```\nmsiexec /i zeromq-4.3.3-Win64-vc142-libpgm-5.3.0.msi /quiet\n```\n\n4. 验证 ZeroMQ 是否已正确安装。在命令提示符或 PowerShell 中运行以下命令：\n\n```\nwhere zmq.dll\n```\n\n如果命令输出 ZeroMQ 的安装路径，则说明 ZeroMQ 已正确安装。\n\n希望这可以帮助您在 Windows 上使用命令行安装 ZeroMQ。"
        if self.question_id == 0:
            with open("安装tornado-02.txt", "r", encoding="utf8") as f:
                rtn_message = f.read()
                # rtn_message = "您可以按照以下步骤在 Windows 上使用命令行安装 ZeroMQ：\n\n1. 下载 ZeroMQ 的二进制文件，例如：https://download.opensuse.org/repositories/network:/messaging:/zeromq:/git-stable/Windows/\n\n2. 打开命令提示符或 PowerShell 并将当前目录更改为 ZeroMQ 二进制文件所在的目录。\n\n3. 运行以下命令，安装 ZeroMQ：\n\n```\nmsiexec /i zeromq-{version}-Win64-vc{vc_version}-libpgm-{pgm_version}.msi /quiet\n```\n其中，`{version}` 是 ZeroMQ 版本，在文件名中查找；`{vc_version}` 是 Visual Studio 版本，例如 2019 为 `14`，2017 为 `15`；`{pgm_version}` 是 PGM 协议库的版本，需要与 ZeroMQ 版本匹配。例如，如果您下载了 `zeromq-4.3.3-Win64-vc142-libpgm-5.3.0.msi`，则应该运行：\n\n```\nmsiexec /i zeromq-4.3.3-Win64-vc142-libpgm-5.3.0.msi /quiet\n```\n\n4. 验证 ZeroMQ 是否已正确安装。在命令提示符或 PowerShell 中运行以下命令：\n\n```\nwhere zmq.dll\n```\n\n如果命令输出 ZeroMQ 的安装路径，则说明 ZeroMQ 已正确安装。\n\n希望这可以帮助您在 Windows 上使用命令行安装 ZeroMQ。"
        else:
            with open("使用wget下载zeromq.txt", "r", encoding="utf8") as f:
                rtn_message = f.read()
        self.question_id += 1

        rtn_msg = json.dumps({"err_code": ErrorCode.Success, "answer": rtn_message})
        await self.write_message(rtn_msg.encode("utf8"))

    def on_close(self):
        pass


async def main():
    # 初始化websocket server
    app = tornado.web.Application([
        (r"/submit", MainServer)
    ])
    app.listen(8000)
    await asyncio.Event().wait()


if __name__ == '__main__':
    asyncio.run(main())
