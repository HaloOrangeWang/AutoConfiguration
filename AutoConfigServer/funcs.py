import re


def get_steps(answer_str: str) -> int:
    """获取原始答案中的步数"""
    answer_lines = answer_str.split("\n")
    command_id = 0
    for line in answer_lines:
        match_res = re.match(r"(?P<id2>[0-9]+)\.", line)
        if match_res:
            id2 = int(match_res.groupdict()["id2"])
            if id2 == command_id + 1:
                command_id += 1
            else:
                raise ValueError("GPT返回的序号不准确")
    if command_id == 0:
        ValueError("GPT没有返回序号")
    return command_id
