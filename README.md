# Trongo: 波场极光 (Open Source & Secure)

> **⚠️ 特别安全声明 (Security Notice)**
>
> 2025-12-23: 本代码库已由专门的安全流程进行 **彻底清洗 (Sanitized)**，移除了原版附带的盗号后门。
> 最新版本（v2.0）深度重构了系统架构：
> - **[引入] C++ 原生 Telegram Bot 托管系统**：为了实现安全的云端多端操控，我们重新引入了 `libcurl`，**但仅限于向 `api.telegram.org` 通信**，绝不会向任何第三方不可信服务器发送私钥。全自动化长轮询，纯净无后门。
> - **[新增] 一键傻瓜式向导管理器 (`tron`)**：告别繁琐的长串参数，支持一键拉起本地集群或托管至云端机器人。
> - **[修复]** 剥离了固定的 Debug 种子器，强制使用真随机硬件种子，保证资产绝对安全。

---

全新一代波场（TRON）靓号生成器，利用 `gpu` (OpenCL) 进行极速破解加速。代码全汉化、抗乱码、支持双擎并行。🔥🔥🔥

## 🚀 极速部署 (Debian / Ubuntu / Linux)

### 1. 准备环境库
除了编译必备环境外，本程序在 Linux 下依赖于 `libcurl`（用于 TG 通知协议）：
```bash
sudo apt update
sudo apt install build-essential ocl-icd-opencl-dev clinfo libcurl4-openssl-dev
```

### 2. 编译项目
```bash
# 获取源码后，在项目根目录一键编译：
make
```

---

## 🎮 玩法指北：使用交互式向导 (`tron`)

编译成功后，我们强烈推荐抛弃旧版的原始长命令，直接使用项目内置的**极简交互式管家**。

在项目目录中直接运行：
```bash
./tron start
```

### 模式一：💻 纯本地后台挂机 (离线党最爱)
在向导中键入 `1`。程序会通过对话的方式让你设置：
- 匹配前缀？匹配后缀？
全部回答完毕后，引擎会自动脱离终端进入静默运转模式。
*随时想看算力？请输入 `./tron speed`*
*爆出结果了吗？请输入 `./tron result`*

### 模式二：🤖 Telegram 云端机器人托管 (极客/服务器党最爱)
在向导中键入 `2`。向导会要求你输入你在 TG 上申请的 **Bot Token** 和接收通知的 **Chat ID**。
输入后，你就可以彻底合上电脑！
- 手机打开 Telegram 发送指令，Bot 会弹出**图形化按钮菜单**。
- 按下【🚀 启动挂机】，在聊天框直接调整配置，服务器瞬间爆转。
- 按下【⚡ 查算力】，Bot 会将当前显卡的算力进度发送给你。
- **一旦抢出极品靓号，Bot 会立刻将地址推送到你的手机！**

---

## 🏷 向导基础命令大全

运行 `./tron` 即可呼出帮助面板：
*   `./tron start`   - 发起一个新的任务向导 (支持换参数)
*   `./tron stop`    - 紧急拉停，杀死所有后台的 Trongo 算力怪兽
*   `./tron restart` - 旧任务作废，立刻重启一个新的向导
*   `./tron -s`      - 查看当前引擎哈希速度
*   `./tron -r`      - 读取已爆出的靓号记事本 (`result.txt`)

---

## 🖥 传统命令 (硬核原教旨主义者)

如果你想把本程序集成进你的其他 Shell 脚本中，也可以直接暴露运行：

```bash
# 1. 基础用法 (读取 profanity.txt 规则，前缀1位，后缀8位)
./profanity.x64 --matching profanity.txt --prefix-count 1 --suffix-count 8

# 2. 指定单个长地址规则 (例如找 TUqE 开头)
./profanity.x64 --matching TUqEg3dzVEJNQSVW2HY98z5X8SBdhmao8D --prefix-count 4 --suffix-count 0

# 3. 结果保存到 result.txt 并跳过显卡设备 1
./profanity.x64 --matching profanity.txt --output result.txt --skip 1
```

*(原版帮助文档详见 `./profanity.x64 --help`)*

---

## 🛡️ Windows 与 Mac 用户

### Windows 用户 (Visual Studio)
1. 安装 Visual Studio (包括 C++ 桌面开发)。
2. 本项目已经重新引入 `libcurl`，如需在 Windows 下编译，请确保引入了 `libcurl` 相应的 `include` 目录与 `.lib` 静态链接库。
3. 双击 `profanity.sln`，选择 `Release` -> `x64` -> `生成`。

### Mac 用户 (M-series / Intel)
在 Mac 下因为框架自带网络库，直接输入 `make` 很大几率能开箱即用。
```bash
make
./tron start
```

---

## 验证地址
无论使用什么工具，生成的私钥务必进行匹配验证。
验证地址：[https://secretscan.org/PrivateKeyTron](https://secretscan.org/PrivateKeyTron)

> **声明**: 本程序仅供技术安全研究探讨。请妥善冷备份您的私钥，由于私钥泄露导致的任何资产损失作者不承担责任。项目源码基于 [johguse/profanity](https://github.com/johguse/profanity) 魔改进化。