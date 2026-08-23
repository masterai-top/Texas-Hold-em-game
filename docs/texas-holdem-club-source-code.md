# 德州扑克俱乐部源码说明

当前公开内容以 C++/Tars 比赛服务端为主，包括 Match、Order、牌局生命周期、计时器、房间消息和奖励配置。

## 阅读顺序

1. `MatchProto.tars` 与 `MatchServant.tars`
2. `MatchServer.cpp` 与 `MatchServantImp.cpp`
3. `Processor.cpp`、`gamebegin.h`、`gamecalculate.h`、`gameend.h`
4. `onclientmessage.h`、`onroommessage.h`、`sendroommessage.h`
5. 奖励配置与 Order 服务

[前往 GitHub 下载源码](https://github.com/masterai-top/Texas-Holdem-Poker-Game-Server-Club-Source-Code)

`Packages/` 和 `ProjectSettings/` 不能单独证明完整 Unity 客户端已公开，具体范围以实际文件为准。

