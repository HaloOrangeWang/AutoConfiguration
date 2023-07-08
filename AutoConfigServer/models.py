from settings import *
import pymongo
from typing import List, Optional
import datetime


class SqlModel:
    """管理数据库连接的"""

    def __init__(self):
        conn = pymongo.MongoClient(DB_SERVER)
        self.db = conn[DB_NAME]

    def get_gpt_keys(self) -> List:
        """获取数据库中存储的全部Gpt Key"""
        api_key_list = []
        col = self.db["GptApiKey"]
        cursor = col.find()  # 返回一个游标，需要遍历获取所有数据
        for doc in cursor:
            api_key_list.append(doc["api_key"])
        return api_key_list

    def get_next_question_id(self):
        """获取下一个question的id号"""
        updated_count_doc = self.db["Counter"].find_one_and_update({'_id': "question"},
                                                                   {'$inc': {'seq_value': 1}},
                                                                   upsert=True, return_document=True)
        return updated_count_doc['seq_value']

    def store_first_question(self, question: str) -> int:
        """创建一个问题并存储，更新问题的id"""
        # 创建question结构体
        curtime = datetime.datetime.now()
        data_doc = {"question": question, "submit_time": curtime}
        # 给这个数据创建自增id
        next_id = self.get_next_question_id()
        data_doc["_id"] = next_id
        self.db["Questions"].insert_one(data_doc)
        # 返回问题的id
        return next_id

    def store_next_question(self, question_id: int, question: str):
        """存储客户端追加的提问"""
        self.db["Questions"].update_one({'_id': question_id},
                                        {'$push': {'next_questions': question}})

    def update_first_answer(self, question_id: int, answer: Optional[str]):
        """存储一个问题的回答"""
        answer_str = 'Exception' if answer is None else answer
        self.db["Questions"].update_one({'_id': question_id},
                                        {'$set': {'answer': answer_str}})

    def update_next_answer(self, question_id: int, answer: Optional[str]):
        """存储客户端追加的提问的答案"""
        answer_str = 'Exception' if answer is None else answer
        self.db["Questions"].update_one({'_id': question_id},
                                        {'$push': {'next_answers': answer_str}})

    def store_feedback(self, feedback: str, question: str, answer: str, bugreport: str):
        # 创建feedback结构体
        curtime = datetime.datetime.now()
        data_doc = {"feedback": feedback, "question": question, "answer": answer, "bugreport": bugreport, "submit_time": curtime}
        self.db["Feedbacks"].insert_one(data_doc)
