## 服务端程序（Python）运行方法

1. 安装依赖库 `pip install -r requirements_server.txt`

2. 需要安装 `mongodb`

3. 服务端程序需要至少一个OpenAI api key才能启动。需要向 `mongodb` 中插入至少一个api key，依次输入命令：

```
mongo
use AutoConfig
db.GptApiKey.insert({"api_key": "**你的OpenAI api key**");
```

4. 下载 `bootstrap-3.4.1-dist` 和 `jquery-3.6.0.js` 放到 `AutoConfigServer/frontend/static` 文件夹下