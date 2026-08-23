# 构建与 Unity 完整性检查

## 服务端

```bash
git clone https://github.com/masterai-top/Texas-Holdem-Poker-Game-Server-Club-Source-Code.git
cd Texas-Holdem-Poker-Game-Server-Club-Source-Code
make
```

构建前确认 Linux、GCC/G++、Tars、头文件、链接库与配置路径。失败时记录第一处错误和完整环境信息。

## Unity

完整 Unity 项目通常至少需要 `Assets/`、`Packages/manifest.json` 和 `ProjectSettings/ProjectVersion.txt`。若缺少 `Assets/`，通常不能构建完整客户端。README 应明确当前公开范围，不应只因存在 `Packages/` 和 `ProjectSettings/` 就宣称完整客户端可运行。

