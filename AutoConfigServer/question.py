from settings import *
from funcs import get_steps


install_prompt = """在{env_os}{env_sw}环境下，使用命令行安装{package}。

一、需要注意：
1. 最好使用一种包版本管理工具。如果它不是自带的（如pip），需先给出命令验证该工具是否已安装，再给出安装它的命令。验证和安装写在同一个步骤中。
2. 每一个步骤应当包含目的说明、命令输入的地址（cmd或powershell或某个文件）、命令内容。不需要举例子。
3. 完成后不需要使用它，也不需要验证安装结果。
{venv_prompt}

二、输出方式：
输出格式为Markdown。每一步命令都应该有序号，命令内容以```标记出来。
以windows平台上用命令行安装Python 3.8为例。以下[[ ]]中的内容为示例内容：[[
1. 安装Chocolatey
 - 目的：检查Chocolatey是否已安装
 - 地址：CMD
 - 命令内容
  ```
  choco --version
  ```
 - 目的：如果Chocolatey尚未安装，则输入如下命令安装：
 - 地址：Powershell
 - 命令内容
  ```
  Set-ExecutionPolicy Bypass -Scope Process -Force; iex ((New-Object System.Net.WebClient).DownloadString('https://chocolatey.org/install.ps1'))
  ```

2. 安装Python 3.8
 - 目的：安装Python 3.8
 - 地址：CMD
 - 命令内容
  ```
  choco install python --version=3.8 -y
  ```

到此为止。不需要使用python 3.8。
]]
该示例中使用chocolatey这个工具。安装其他库时，也可以根据实际情况使用其他的包管理工具。
"""

config_prompt = """在{env_os}{env_sw}环境下，使用命令行配置{package}{description}。

一、需要注意：
1. 如果需要安装依赖包，需先给出命令验证该依赖包是否已安装，再给出安装它的命令。验证和安装写在同一个步骤中。且最好使用一种包版本管理工具来安装。
2. 每一个步骤应当包含目的说明、命令输入的地址（cmd或powershell或某个文件）、命令内容。如果命令输入的地址是某个文件，不需要额外给出打开该文件的命令。不需要举例子，但可以解释命令中一些“名称”的含义。
3. 完成后不需要使用它，也不需要验证操作结果。
{venv_prompt}

二、输出方式：
输出格式为Markdown。每一步命令都应该有序号，命令内容以```标记出来。
以windows平台上用命令行配置mysql默认字符编码为例。以下[[ ]]中的内容为示例内容：[[
1. 安装mysql
 - 目的：检查mysql是否已安装
 - 地址：CMD
 - 命令内容
  ```
  mysql --version
  ```
 - 目的：如果mysql尚未安装，则输入如下命令安装：
 - 地址：CMD
 - 命令内容
  ```
  choco install mysql
  ```
2. 配置MySQL默认编码为utf8
 - 目的：配置MySQL默认编码为utf8
 - 地址：MySQL的配置文件 my.ini
 - 命令内容
  ```
  [mysqld]
  character-set-server=utf8
  collation-server=utf8_general_ci
  ```
3. 重启mysql服务
 - 目的：使配置生效
 - 地址：CMD
 - 命令内容
  ```
  net stop mysql
  net start mysql
  ```
]]
"""


def format_origin_question(question_dict: dict):
    """将客户端输入的问题（json形式）转换成给openai提交的形式"""
    question_type = int(question_dict["type"])
    # 生成“软件环境”的相关prompt
    env_sw_prompt = ""
    for env_sw in question_dict["env_sw"]:
        env_sw_prompt += "，%s" % env_sw
    # 生成"是否使用docker和venv“的prompt
    venv_option = int(question_dict["venv_option"])
    if venv_option == VenvOption.VenvForbid:
        venv_prompt = "4. 不使用容器和虚拟环境。"
    elif venv_option == VenvOption.VenvAllow:
        venv_prompt = ""
    else:
        venv_prompt = "4. 建议使用容器或虚拟环境，如docker、devcontainers、virtualenv等。"
    # 生成最终的Prompt
    if question_type == OrderType.OrderInstall:
        # 这是个安装问题
        prompt = install_prompt.format(env_os=question_dict["env_os"],
                                       env_sw=env_sw_prompt,
                                       package=question_dict["package"],
                                       venv_prompt=venv_prompt)
    else:
        # 这是个配置问题
        prompt = config_prompt.format(env_os=question_dict["env_os"],
                                      env_sw=env_sw_prompt,
                                      package=question_dict["package"],
                                      description=question_dict["description"],
                                      venv_prompt=venv_prompt)
    return prompt


check_install_prompt = """询问{origin_question}，得到的回答在下文[[ ]]中。
[[
{origin_answer}
]]

该回答包含了若干步骤，按作用可以分成如下几类：
a. 安装依赖项和包版本管理工具等。
b. 实现{origin_question}的核心安装命令。
c. 安装后的配置操作，如添加环境变量等。
d. 只是对安装结果的验证、使用和测试。
e. 其他。

请先对原回答中的全部{step_cnt}个步骤，按作用进行分类。

然后给出“只是对安装结果的验证、使用和测试”的步骤在原回答中的序号列表。输出格式为列表即可。示例：[3,4,5]。如果原回答中没有该类型的步骤，则返回空列表：[]。
"""

check_config_prompt = """询问{origin_question}，得到的回答在下文[[ ]]中。
[[
{origin_answer}
]]

该回答包含了若干步骤，按作用可以分成如下几类：
a. 配置前的准备命令，如安装依赖项等。
b. 实现{origin_question}的核心配置命令。
c. 配置后，使配置生效的命令。
d. 只是对配置结果的验证、使用和测试。
e. 其他。

请先对原回答中的全部{step_cnt}个步骤，按作用进行分类。

然后给出“只是对配置结果的验证、使用和测试”的步骤在原回答中的序号列表。输出格式为列表即可。示例：[3,4,5]。如果原回答中没有该类型的步骤，则返回空列表：[]。
"""


def format_check_question(question_dict: dict, raw_answer: str) -> str:
    """
    生成“去除检查环节”的prompt
    :param question_dict: 原始问题（json格式）
    :param raw_answer: GPT给出的原始答案
    :return: 生成让GPT判断“检查环节”的prompt内容
    """
    question_type = int(question_dict["type"])
    step_cnt = get_steps(raw_answer)  # 原始回答中一共有多少个步骤
    if question_type == OrderType.OrderInstall:
        # 这是个安装问题
        # 1.生成原始问题的字符串
        env_sw_prompt = ""
        for env_sw in question_dict["env_sw"]:
            env_sw_prompt += "，%s" % env_sw
        origin_question = "在{env_os}{env_sw}环境下，使用命令行安装{package}"\
            .format(env_os=question_dict["env_os"],
                    env_sw=env_sw_prompt,
                    package=question_dict["package"])
        # 2.生成正式的prompt
        prompt = check_install_prompt.format(origin_question=origin_question,
                                             origin_answer=raw_answer,
                                             step_cnt=step_cnt)
    else:
        # 这是个配置问题
        # 1.生成原始问题的字符串
        env_sw_prompt = ""
        for env_sw in question_dict["env_sw"]:
            env_sw_prompt += "，%s" % env_sw
        origin_question = "在{env_os}{env_sw}环境下，使用命令行配置{package}{description}"\
            .format(env_os=question_dict["env_os"],
                    env_sw=env_sw_prompt,
                    package=question_dict["package"],
                    description=question_dict["description"])
        # 2.生成正式的prompt
        prompt = check_config_prompt.format(origin_question=origin_question,
                                            origin_answer=raw_answer,
                                            step_cnt=step_cnt)
    return prompt
