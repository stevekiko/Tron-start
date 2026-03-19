# Trongo

> GPU 加速的 TRON 波场靓号地址生成器，原生集成 Telegram Bot 远程管控。

基于 [profanity-tron](https://github.com/johguse/profanity) 深度改造，新增原生 C++ Telegram Bot 守护进程实现远程控制与爆号通知、交互式 CLI 管理脚本 `tron`、以及 AES-256 私钥文件加密存储——零外部脚本语言依赖。

---

## 功能特性

- **GPU 并行加速** — OpenCL 多显卡并发计算，性能拉满
- **原生 Telegram Bot** — 从手机一键启停引擎、查看算力、接收爆号推送，按钮交互无需打命令
- **常驻快捷键盘** — Telegram 聊天底部永久驻留操作按钮，告别 `/start` 唤醒
- **远程规则切换** — 直接在 Telegram 选择豹子数字 / 字母 / 全集规则库，一键切换
- **AES-256 私钥加密** — 结果文件使用 `openssl enc -aes-256-cbc -pbkdf2` 加密写入，服务器入侵也无法读取
- **交互式 CLI 向导** — `tron` 命令涵盖所有操作，向导式引导，零学习成本
- **双执行模式** — 纯本地单机模式 或 Telegram 云端全托管模式可选

---

## 环境依赖

### Linux（推荐部署平台）

| 依赖 | 用途 | 安装命令 |
|---|---|---|
| `git` | 代码拉取 | `apt install git` |
| `g++`、`make` | C++ 编译工具链 | `apt install build-essential` |
| `libcurl4-openssl-dev` | Telegram API 网络通信 | `apt install libcurl4-openssl-dev` |
| `ocl-icd-opencl-dev` | OpenCL 头文件 | `apt install ocl-icd-opencl-dev` |
| `openssl` | 私钥文件加密解密 | Debian/Ubuntu 预装 |
| 显卡 OpenCL 驱动 | NVIDIA 或 AMD GPU 计算 | 见下方驱动安装章节 |

### macOS

Xcode Command Line Tools + Homebrew 可满足所有依赖。`libcurl` 由系统提供。

### Windows

使用 Visual Studio 构建，需自行配置 `libcurl` 的 include 路径和 `.lib` 链接，然后直接打开 `profanity.sln`，选择 `Release x64` 生成。

---

## 部署教程（Linux 完整步骤）

### 第一步：安装系统依赖

```bash
sudo apt update
sudo apt install -y git build-essential libcurl4-openssl-dev ocl-icd-opencl-dev clinfo
```

### 第二步：安装显卡 OpenCL 驱动

**NVIDIA 显卡：**

```bash
sudo apt install -y nvidia-opencl-icd
```

**AMD 显卡：**

```bash
sudo apt install -y mesa-opencl-icd
```

验证显卡是否被正确识别：

```bash
clinfo | grep "Device Name"
```

输出中应看到你的 GPU 型号。若无输出，请检查驱动安装是否正确。

### 第三步：克隆项目

```bash
git clone https://github.com/stevekiko/Trongo.git
cd Trongo
```

### 第四步：编译

```bash
make
```

编译完成后当前目录会生成 `profanity.x64` 核心引擎，`tron` 脚本权限会自动设置好。

### 第五步：注册全局命令

```bash
sudo ln -sf $(pwd)/tron /usr/local/bin/tron
```

完成后在服务器**任意目录**均可直接使用 `tron` 命令。

---

## 使用方式

### 启动向导

```bash
tron start
```

向导将引导你完成以下配置：

1. **运行模式选择** — 纯本地模式 或 Telegram Bot 云托管模式
2. **匹配规则配置** — 规则文件路径、前缀/后缀匹配位数
3. **私钥加密设置** — 可选 AES-256 加密密码（强烈建议开启）

### 全部命令速查

```
tron start      启动向导（本地或 Telegram 托管模式）
tron stop       停止所有正在运行的挖号进程
tron restart    停止 + 重新启动向导
tron -s         查看当前实时哈希速率
tron -r         查看爆号结果（若已加密则提示输入解密密码）
```

---

## Telegram Bot 模式

### 前置准备

1. 在 Telegram 搜索 [@BotFather](https://t.me/BotFather)，创建专属机器人，获取 **Bot Token**
2. 获取你的 **Chat ID** — 向机器人发任意消息，然后访问：
   `https://api.telegram.org/bot<TOKEN>/getUpdates`
   在返回 JSON 中找到 `"id"` 字段即为 Chat ID

### 启动

执行 `tron start`，选择模式 **2**，按提示填入 Bot Token 和 Chat ID。配置会自动保存到 `tg_config.txt`，下次启动直接复用，无需重新配置。

### 常驻快捷键盘

引擎启动后，机器人会向聊天推送永久驻留键盘，关掉再开还在：

```
┌────────────────┬────────────────┐
│  🚀 启动挂机   │  🎯 设置规则   │
├────────────────┼────────────────┤
│  ⚡ 查算力     │  🏆 查结果     │
├────────────────┴────────────────┤
│  🔴 紧急停止   │  📋 主菜单     │
└────────────────┴────────────────┘
```

### 远程规则切换

点击 **🎯 设置规则** 按钮，选择预置规则库：

| 按钮 | 匹配内容 |
|---|---|
| 🔢 纯数字连号 | 后缀连续数字 1–9 各一套 |
| 🔠 纯字母连号 | 后缀连续字母 A–Z 各一套 |
| 👑 数字+字母全集 | 以上所有规则合并 |

也可直接在聊天框发送多行文本（每行一条 20 字符前缀规则），机器人会自动覆写 `profanity.txt` 并弹出难度选择菜单。

---

## 规则文件格式

`profanity.txt` 每行一条规则，支持两种格式：

- **20 字符匹配串** — 直接用于前缀或后缀比对
- **34 字符完整 TRON 地址** — 自动截取头尾 10 字符作为模糊匹配

示例：
```
TTTTTTTTTT8888888888
TTTTTTTTTTAAAAAAAAAA
TUqEg3dzVEJNQSVW2HY98z5X8SBdhmao8D
```

---

## 私钥文件加密说明

开启加密后，每次爆出靓号，私钥会通过以下流程安全写入：

1. 解密现有结果文件（若已存在）→ 临时明文
2. 追加新私钥条目  
3. AES-256-CBC + PBKDF2 重新加密
4. 删除临时明文，只保留密文

**通过 `tron -r` 查看：**
```bash
tron -r
# 🔐 检测到私钥文件已加密，请输入解密密码:
```

**无需 tron，手动用 openssl 解密：**
```bash
openssl enc -d -aes-256-cbc -pbkdf2 -in result.txt -pass pass:你的密码
```

未配置密码时，结果以明文 CSV 格式追加写入：`私钥,地址`

---

## 进阶：直接调用底层引擎

如需集成到脚本或多机集群，可绕过 `tron` 直接调用 `profanity.x64`：

```bash
# 本地模式 — 规则文件 + 前缀0位 + 后缀6位
./profanity.x64 --matching profanity.txt --prefix-count 0 --suffix-count 6 --output result.txt

# 本地模式 + 私钥加密输出
./profanity.x64 --matching profanity.txt --suffix-count 8 --output result.txt --result-key "你的密码"

# 单地址精准模糊匹配
./profanity.x64 --matching TUqEg3dzVEJNQSVW2HY98z5X8SBdhmao8D --prefix-count 4

# Telegram 守护进程模式（引擎空转等待远程指令）
./profanity.x64 --tg-token "TOKEN" --tg-chat "CHAT_ID"

# Telegram 守护进程 + 加密输出
./profanity.x64 --tg-token "TOKEN" --tg-chat "CHAT_ID" --result-key "你的密码" --output result.txt
```

完整参数列表：

```
-h / --help             显示帮助
-m / --matching         规则文件路径或 34 字符地址串
-o / --output           结果输出文件路径
-b / --prefix-count     前缀匹配位数（最大 10）
-e / --suffix-count     后缀匹配位数（最大 10）
-w / --work             OpenCL 本地工作组大小（默认 64）
-n / --no-cache         禁用 OpenCL 内核二进制缓存
-T / --tg-token         Telegram Bot Token（触发守护进程模式）
-C / --tg-chat          Telegram 通知目标 Chat ID
-K / --result-key       结果文件 AES-256 加密密码
```

---

## 安全说明

- **网络访问仅限 `api.telegram.org`** — 不存在任何其他出站连接，全部历史后门代码（远程 POST 上报、调试固定种子）已彻底清除
- **私钥不会向 Telegram 发送** — Bot 推送通知仅包含爆出的靓号地址，私钥只写入本地文件
- **结果文件加密** — 使用 AES-256-CBC + PBKDF2 标准格式，密码不经过任何网络传输
- **随机数安全** — 已修复原版 profanity 存在的固定种子漏洞，现使用系统级熵源

---

## 目录结构

```
Trongo/
├── profanity.cpp          主入口：CLI 解析、生命周期管理
├── Dispatcher.cpp/.hpp    OpenCL 设备管理与 GPU 内核调度
├── TGBot.cpp/.hpp         原生 C++ Telegram Bot（长轮询、键盘、回调）
├── Mode.cpp/.hpp          规则解析与地址匹配逻辑
├── ArgParser.hpp          命令行参数解析器
├── SpeedSample.cpp/.hpp   哈希速率采样
├── json.hpp               nlohmann/json（单头文件，已打包）
├── kernel_profanity.hpp   内嵌 OpenCL 内核（主引擎）
├── kernel_keccak.hpp      内嵌 OpenCL 内核（Keccak-256）
├── kernel_sha256.hpp      内嵌 OpenCL 内核（SHA-256）
├── tron                   交互式 CLI 管理脚本
├── profanity.txt          默认靓号匹配规则文件
└── Makefile
```

---

## 开源许可

本项目遵循 MIT 协议开源。

底层算力引擎源自 [johguse/profanity](https://github.com/johguse/profanity)。Telegram 集成、加密存储及 CLI 工具由 [stevekiko](https://github.com/stevekiko) 贡献。