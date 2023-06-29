from concurrent.futures import ThreadPoolExecutor
import threading
from typing import Optional
import openai
import os


def output_answer(f):
    def call(cls, question: str):
        answer = f(cls, question)
        if answer is not None:
            if not os.path.exists("answers"):
                os.makedirs("answers")
            t = 0
            for t in range(10000):
                if not os.path.exists("answers/%04d.txt" % t):
                    break
            f2 = open("answers/%06d.txt" % t, "w", encoding="utf8")
            f2.write(question + "\n\n\n")
            f2.write(answer)
            f2.close()
        return answer
    return call


class GptClient:

    def __init__(self, gpt_api_keys: list):
        self.threadpool = ThreadPoolExecutor(max_workers=len(gpt_api_keys))  # 执行任务的线程
        self.gpt_api_keys = gpt_api_keys
        # self.next_api_key_dx = 0
        # self.lock = threading.Lock()
        # self.thread_to_gpt_id = dict()  # 对于线程池中的每个线程，它的id号与GPT账号之间的对应关系
        openai.api_key = self.gpt_api_keys[0]

    def init_thread(self):
        """线程初始化。初始化时主要做的事情是确认gpt api key和线程id之间的对应关系"""
        # self.lock.acquire()
        # self.thread_to_gpt_id[self.next_api_key_dx] = self.gpt_api_keys[self.next_api_key_dx]
        # self.next_api_key_dx += 1
        # self.lock.release()

    # noinspection PyMethodMayBeStatic
    @output_answer
    def submit_question(self, question: str) -> Optional[str]:
        """将问题提交给GPT。并等待答案"""
        messages = [
            {"role": "system", "content": "you are a programmer."},
            {"role": "user", "content": question}
        ]
        # noinspection PyBroadException
        try:
            response = openai.ChatCompletion.create(model="gpt-3.5-turbo", messages=messages)
            return response["choices"][0]["message"]["content"]
        except Exception:
            return None
