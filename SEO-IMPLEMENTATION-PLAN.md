# 德州扑克俱乐部服务端 SEO 优化方案

## 当前问题

1. README 和 About 重复堆叠“德州源码”，营销内容多于可验证工程信息。
2. 宣称完整 Unity/Cocos、H5、AI、数据库和可直接商业运营，但公开目录未显示完整客户端 `Assets/`、数据库脚本或 AI 模块。
3. `poker-ai`、`texas-holdem-ai`、`unity3d` Topics 与当前公开内容不完全匹配。
4. 根目录提交 `.d` 构建依赖文件，缺少有效 `.gitignore`。
5. 缺少可复现依赖、配置、测试、CI 和 Release。
6. 截图不能替代源码范围、构建状态和使用文档。

## 建议增加

```text
.
|-- README.md
|-- License.md
|-- .gitignore
|-- CONTRIBUTING.md
|-- SECURITY.md
|-- CHANGELOG.md
|-- docs/
|   |-- texas-holdem-club-source-code.md
|   |-- server-architecture.md
|   |-- build-guide.md
|   |-- match-game-flow.md
|   |-- tars-message-guide.md
|   |-- security-compliance.md
|   `-- faq.md
|-- config/example/          # 脱敏配置
|-- tests/                   # 匹配、状态与奖励测试
|-- examples/                # 协议示例
|-- benchmarks/              # 可复现性能数据
`-- .github/workflows/       # CI 构建
```

## 工程优先级

- 说明操作系统、编译器、Tars 和依赖版本。
- 补充脱敏配置、服务端口、依赖和启动顺序。
- 增加重复匹配、超时、断线、重复结算、奖励更新和订单幂等测试。
- 若公开 Unity 客户端，补齐 `Assets/` 并注明 Unity 版本；否则明确只含配置片段。
- 将 `.d`、对象文件和本机构建产物加入 `.gitignore`。
- 创建带源码范围、构建状态和校验值的 Release。
- 性能与 AI 声明必须附实现入口和可复现证据。

## 搜索与下载转化

主关键词使用 `德州源码` 和 `德州扑克源码`；差异化关键词使用 `德州俱乐部源码`、`扑克比赛服务端`、`C++ Tars 匹配服务`。这与 `德州源码 site:github.com` 的核心搜索场景一致。 :codex-annotation{index="1"}

README 首屏应回答“公开了什么、如何下载、能否构建”。docs 分别覆盖架构、构建、匹配、协议和安全。不要复制相同正文批量换关键词。

建立 GitHub Release，并用 GitHub Pages 发布文档站，配置 title、description、canonical、sitemap、robots.txt 和 `SoftwareSourceCode` 结构化数据，再提交 Google Search Console。

## 执行顺序

1. 替换 README、About 和 Topics。
2. 明确服务端与 Unity 客户端的实际公开范围。
3. 添加 `.gitignore`、构建、配置、测试和安全文档。
4. 增加 CI 并发布首个规范 Release。
5. 部署文档站并提交 Google 收录。

