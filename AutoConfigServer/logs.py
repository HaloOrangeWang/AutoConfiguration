import logging
import os
from typing import Optional


class Log:

    logger: Optional[logging.Logger] = None

    @classmethod
    def init(cls):
        if not os.path.exists("./log"):
            os.mkdir("./log")
        cls.logger = logging.getLogger()
        cls.logger.setLevel(logging.DEBUG)  # or whatever
        handler = logging.FileHandler('./log/log.txt', 'a', 'utf-8')
        handler.setFormatter(logging.Formatter('%(asctime)s %(levelname)s %(message)s'))
        cls.logger.addHandler(handler)

    @classmethod
    def print_log(cls, s: str):
        cls.logger.info(s)
