# 比赛服务端架构

```text
客户端/房间消息
       |
       v
MatchServant -> Processor -> gamebegin/gamecalculate/gameend
       |                         |
       v                         v
TimerThread                 room messages
       |
       v
reward config / Order service
```

这是源码阅读索引，不代表完整生产拓扑。实际部署前应确认线程模型、对象生命周期、服务发现、持久化、超时、重试和启动顺序。

协议、业务状态、数据访问和日志应保持分离，每个状态转换都应有测试。

