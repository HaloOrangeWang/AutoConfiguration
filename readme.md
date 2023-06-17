## 服务端程序（Python）运行方法

1. 安装依赖库 `pip install -r requirements_server.txt`

2. 服务端程序需要至少一个OpenAI api key才能启动。需要向 `mysql` 中插入至少一个api key，依次输入命令：

```
mysql -u root -p123456
create database AutoConfigure;
use AutoConfigure;
create table GptApiKey(id integer primary key auto_increment, api_key text not null);
insert into GptApiKey (api_key) values ("**你的OpenAI api key**");
```

