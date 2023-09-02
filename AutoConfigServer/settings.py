from enum import IntEnum


DB_SERVER = "mongodb://localhost:27017"
DB_NAME = "AutoConfig"


class OrderType(IntEnum):
    """指令类型：安装或配置"""
    OrderInstall = 0
    OrderConfig = 1


class VenvOption(IntEnum):
    """是否允许容器和虚拟环境"""
    VenvForbid = 0
    VenvAllow = 1
    VenvCommand = 2
