# Trongo: 波场极光 (Open Source & Secure)

> **⚠️ 特别安全声明 (Security Notice)**
>
> 2025-12-23: 本代码库已由专门的安全流程进行 **彻底清洗 (Sanitized)**，移除了原版附带的盗号后门。
> 最新版本（v2.0）深度重构了系统架构：
> - **[引入] C++ 原生 Telegram Bot 托管系统**：为了实现安全的云端多端操控，我们重新引入了 `libcurl`，**但仅限于向 `api.telegram.org` 通信**，绝不会向任何第三方不可信服务器发送私钥。全自动化长轮询，纯净无后门。
> - **[新增] 一键傻瓜式向导管理器 (`tron`)**：告别繁琐的长串命令行参数，小白也能轻松一键挂机。
> - **[修复]** 剥离了固定的 Debug 种子器，强制使用真正的硬件随机种子，保证资产绝对安全。

---

全新一代波场（TRON）极品靓号/地址生成器，利用显卡 `gpu` (OpenCL) 进行极速碰撞加速。代码全汉化、抗乱码、支持双引擎并行。🔥🔥🔥

## 👶 小白专属部署教程 (Debian / Ubuntu)

如果你是一台刚刚购买/重装的 Linux 服务器，请**严格按照以下步骤**逐行复制执行：

### 第一步：准备基础编译环境
我们首先需要安装 C++ 的编译环境和网络通信库：
```bash
sudo apt-update
sudo apt install -y git build-essential libcurl4-openssl-dev
```

### 第二步：安装显卡 (OpenCL) 计算环境
因为本程序是利用**显卡(GPU)**运算的，所以必须安装 OpenCL 相关的驱动和头文件（非常重要，不装无法编译运行）：
```bash
# 1. 先安装通用 OpenCL 头文件包
sudo apt install -y ocl-icd-opencl-dev clinfo

# 2. 根据你的服务器显卡类型，自选一项安装驱动：
# 👉 如果你的服务器是 NVIDIA 显卡 (绝大多数云服务器的选择)：
sudo apt install -y nvidia-opencl-icd
# 👉 如果你的服务器是 AMD 显卡：
sudo apt install -y mesa-opencl-icd
```
> **💡 测试一下：** 安装完成后输入 `clinfo` 并回车，如果屏幕上能输出你的显卡名字（比如 RTX 4090 / 5060），说明环境完美！

### 第三步：下载代码并一键编译
将本项目的代码下载或上传到你的服务器中，进入该目录后执行两行命令：
```bash
# 赋予管家脚本执行权限：
chmod +x tron

# 开始硬核编译（只需执行一次，大约耗时十秒）：
make

# =重要= 将向导程序挂载为全域命令（这样你在任何目录都能直接打 tron）
sudo ln -sf $(pwd)/tron /usr/local/bin/tron
```
看到没有报错提示，目录下多出了一个 `profanity.x64` 文件，并且成功注册了 `tron` 全局命令，即可大功告成！

---

## 🎮 玩法指北：使用交互式向导

编译成功后，彻底抛弃那些反人类的复杂命令代码！你甚至不需要处于当前目录中，只需在你的云服务器任意地方直接执行这行命令：

```bash
tron start
```

屏幕上立刻会弹出一个像问卷一样的纯中文**交互式向导**！

### 模式一：💻 纯本地后台挂机 (离线党最爱)
在弹出的向导询问时，输入数字 `1` 并回车。
- 程序会亲切地问你：想要匹配什么前缀？（比如输入 `1` 代表随意）
- 程序再问你：想要匹配什么后缀？（比输入 `6` 代表豹子号结尾）
回答完毕后，引擎会自动脱离屏幕进入后台“挖矿”。
*随时想看你的显卡算力跑到多快了？敲一下 `tron -s`*
*爆出极品账号了吗？敲一下 `tron -r` 查看记事本*

### 模式二：🤖 Telegram 云端机器人远程托管 (极客最爱)
如果你出门在外想随时查看到底出号了没有，可以在向导时选 `2`。
向导会让你填入在 Telegram 申请到的 **Bot Token** 和 你自己的 **Chat ID**。

就这么简单！这时候你的这台 Linux 服务器就已经变成了一个智能云管家！你彻底可以关掉电脑页面了。
1. 拿起手机，打开 Telegram，向你的专属 Bot 发送消息。
2. 机器人会弹出一排非常漂亮的**图形化按钮**。
3. 点击【🚀 快速挂机】，直接可以在手机聊天页修改算号难度。
4. 点击【⚡ 查算力】，随时随地监控服务器情况。
5. **一旦抢出天选极品靓号，机器人会第一时间闪电弹窗推送给你，直接掉落私钥！**

---

## 🏷 万能命令速查

在服务器代码目录里，任何时候你都可以运行 `tron` 来呼出帮助面板：
*   `tron start`   - 发起一个新的任务向导 (支持换参数)
*   `tron stop`    - 紧急拉闸，干净利落地杀死所有后台挖号进程
*   `tron restart` - 旧任务作废，立刻重启一个新的向导
*   `tron -s`      - 查看当前引擎的哈希速度
*   `tron -r`      - 读取已经爆出大奖的私钥记事本 (`result.txt`)

---

## 🖥 传统命令 (硬核原生极客专供)

如果你偏要将其集成进你自己的集群脚本里，依然可以直接使用底层暴露的 `profanity.x64`：

```bash
# 1. 基础用法 (读取 profanity.txt 规则，前缀1位，后缀8位)
./profanity.x64 --matching profanity.txt --prefix-count 1 --suffix-count 8

# 2. 指定单个长地址规则 (例如找 TUqE 开头)
./profanity.x64 --matching TUqEg3dzVEJNQSVW2HY98z5X8SBdhmao8D --prefix-count 4 --suffix-count 0

# 3. 开启 Telegram 强行后台管家模式
./profanity.x64 --tg-token "你的Token" --tg-chat "你的ID"
```
*(详情可用 `./profanity.x64 --help` 查看全栈参数)*

---

## 🛡️ Windows 与 Mac 环境说明

### Windows 用户 (Visual Studio)
1. 必须安装 Visual Studio (勾选 C++ 桌面开发组件)。
2. 本项目已经重新引入 `libcurl`（用于发送Bot通知），如需在 Windows 下编译，请确保你自己已经配置好了 `libcurl` 对应的 `include` 和 `.lib` 静态链接。
3. 双击 `profanity.sln`，选择 `Release` -> `x64` -> `生成` 即可。

### Mac 用户 (M-series / Intel)
在 Mac 下因为操作系统自带网络库，所以**编译巨简单**。
```bash
make
tron start
```

---

## 防骗与安全验证
无论你用谁写的程序挖号，拿到私钥后，务必经过匹配验证防止“伪造靓号后门”。
官方匹配验证地址：[https://secretscan.org/PrivateKeyTron](https://secretscan.org/PrivateKeyTron)

> **最终声明**: 本程序完全开源，仅供区块链加密安全性研究。由于对私钥极速碰撞，请您务必单机冷备份生成的财富私钥，对任何原因造成的被盗均不承担连带责任。项目演变自传说中的以太坊破译神器 [johguse/profanity](https://github.com/johguse/profanity)。