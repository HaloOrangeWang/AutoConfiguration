from settings import *
from sqlalchemy import create_engine, Column, Integer, Text, DateTime
from sqlalchemy.orm import declarative_base, sessionmaker
import datetime
from typing import List


MysqlUrl = f"mysql+pymysql://{MYSQL_USER}:{MYSQL_PASSWD}@{MYSQL_HOST}:{MYSQL_PORT}/{MYSQL_DB}"
Engine = create_engine(MysqlUrl, pool_recycle=3600)
Base = declarative_base()


class GptApiKey(Base):
    __tablename__ = 'GptApiKey'
    id = Column(Integer, primary_key=True, autoincrement=True)
    api_key = Column(Text, nullable=False)


class Questions(Base):
    __tablename__ = 'Questions'
    id = Column(Integer, primary_key=True, autoincrement=True)
    question = Column(Text, nullable=False)
    submit_time = Column(DateTime, default=datetime.datetime.now)


Base.metadata.create_all(bind=Engine)


class SqlModel:
    """管理数据库连接的"""

    def __init__(self):
        session_ = sessionmaker(Engine)
        self.session = session_()

    def get_gpt_keys(self) -> List:
        """获取数据库中存储的全部Gpt Key"""
        api_key_list = []
        data = self.session.query(GptApiKey).all()
        for item in data:
            api_key_list.append(item.api_key)
        return api_key_list

    def store_question(self, question: str):
        """将一个问题存储到数据库中"""
        question_obj = Questions(question=question)
        self.session.add(question_obj)
        self.session.commit()
