# Trongo

> GPU-accelerated TRON vanity address generator with native Telegram Bot control.

Built on top of [profanity-tron](https://github.com/johguse/profanity), Trongo adds a native C++ Telegram Bot daemon for remote control and real-time notifications, an interactive CLI manager (`tron`), and AES-256 encrypted result storage — all without any external scripting languages.

---

## Features

- **GPU-accelerated address generation** — OpenCL-based parallel computation across multiple GPUs
- **Native Telegram Bot** — Start/stop engine, monitor hash rate, and receive alerts via inline keyboard buttons, all from your phone
- **Persistent keyboard** — Bottom shortcut buttons in Telegram chat; no need to type commands
- **Remote rule configuration** — Switch vanity rule presets (digits / letters / all) directly in Telegram
- **AES-256 result encryption** — Private keys written to disk are encrypted with `openssl enc -aes-256-cbc-pbkdf2`; even if the server is compromised the file is unreadable without your password
- **Interactive CLI manager** — The `tron` command wraps every operation behind a guided wizard
- **Dual execution modes** — Local silent mode or fully Telegram-managed cloud mode
- **Zero scripting language dependencies** — Core bot logic is 100% native C++

---

## Requirements

### Linux (recommended)

| Dependency | Purpose | Install |
|---|---|---|
| `git` | Source code management | `apt install git` |
| `g++`, `make` | C++ compilation toolchain | `apt install build-essential` |
| `libcurl4-openssl-dev` | Telegram API communication | `apt install libcurl4-openssl-dev` |
| `ocl-icd-opencl-dev` | OpenCL headers | `apt install ocl-icd-opencl-dev` |
| `openssl` | Result file encryption | Pre-installed on all Debian/Ubuntu |
| GPU OpenCL driver | NVIDIA or AMD compute | See § GPU Driver |

### macOS

All required libraries ship with Xcode Command Line Tools and Homebrew. `libcurl` is provided by the system.

### Windows

Build with Visual Studio. Configure `libcurl` headers and `.lib` manually in the project settings, then open `profanity.sln` and build in `Release x64`.

---

## Deploy on Linux (Step-by-Step)

### 1. Install system dependencies

```bash
sudo apt update
sudo apt install -y git build-essential libcurl4-openssl-dev ocl-icd-opencl-dev clinfo
```

### 2. Install GPU OpenCL driver

**NVIDIA:**

```bash
sudo apt install -y nvidia-opencl-icd
```

**AMD:**

```bash
sudo apt install -y mesa-opencl-icd
```

Verify detection:

```bash
clinfo | grep "Device Name"
```

You should see your GPU listed. If no devices appear, re-check that the driver is installed correctly.

### 3. Clone the repository

```bash
git clone https://github.com/stevekiko/Trongo.git
cd Trongo
```

### 4. Compile

```bash
make
```

This produces `profanity.x64` and automatically sets the `tron` script as executable.

### 5. Register `tron` as a global command

```bash
sudo ln -sf $(pwd)/tron /usr/local/bin/tron
```

After this step you can run `tron` from any directory.

---

## Usage

### Interactive wizard

```bash
tron start
```

The wizard will guide you through:

1. **Mode selection** — Local mode or Telegram Bot mode
2. **Rule configuration** — Match pattern file, prefix count, suffix count
3. **Encryption setup** — Optional AES-256 password for the result file (strongly recommended)

### All commands

```
tron start      Launch wizard (local or Telegram mode)
tron stop       Gracefully stop all running engine processes
tron restart    Stop + launch wizard again
tron -s         Print current hash rate
tron -r         View results (prompts for decryption password if encrypted)
```

---

## Telegram Bot Mode

### Prerequisites

1. Create a bot via [@BotFather](https://t.me/BotFather) → get Bot Token
2. Get your Chat ID — send any message to the bot, then visit:
   `https://api.telegram.org/bot<TOKEN>/getUpdates`

### Activation

Run `tron start`, select **mode 2**, and enter your Bot Token and Chat ID when prompted. The configuration is saved to `tg_config.txt` so subsequent starts reuse it automatically.

### Persistent keyboard buttons

Once the engine starts, the bot sends a permanent keyboard to the chat:

```
┌────────────────┬────────────────┐
│  🚀 启动挂机   │  🎯 设置规则   │
├────────────────┼────────────────┤
│  ⚡ 查算力     │  🏆 查结果     │
├────────────────┴────────────────┤
│  🔴 紧急停止   │  📋 主菜单     │
└────────────────┴────────────────┘
```

### Rule presets via Telegram

Click **🎯 设置规则** to choose from:

| Button | Rule applied |
|---|---|
| 🔢 纯数字连号 | Suffix-repeated digits 1–9 |
| 🔠 纯字母连号 | Suffix-repeated letters A–Z |
| 👑 数字+字母全集 | All of the above combined |

Alternatively, send any multi-line text directly to the bot to overwrite `profanity.txt` with custom rules (20-character TRON address prefix format).

---

## Matching Rule File Format

`profanity.txt` must contain one rule per line. Each line is either:

- A **20-character** prefix/suffix pattern string
- A **34-character** full TRON address (`T` + 33 base58 characters) — the middle 14 characters are masked for fuzzy matching

Example:
```
TTTTTTTTTT8888888888
TTTTTTTTTTAAAAAAAAAA
TXxxxxxxxxxxxxxxxxxx
```

---

## Result File Encryption

If an encryption password is set during `tron start`, all found private keys are written in AES-256-CBC + PBKDF2 format (standard `openssl enc` format).

**View results:**
```bash
tron -r
# → prompts: 🔐 请输入解密密码:
```

**Manual decryption (no tron required):**
```bash
openssl enc -d -aes-256-cbc -pbkdf2 -in result.txt -pass pass:YOUR_PASSWORD
```

When no password is configured, results are appended as plaintext CSV (`private_key,address`).

---

## Advanced: Direct Engine Invocation

For scripting or cluster integration, `profanity.x64` can be called directly:

```bash
# Local mode — match file, prefix 0, suffix 6 positions
./profanity.x64 --matching profanity.txt --prefix-count 0 --suffix-count 6 --output result.txt

# Local mode with encrypted output
./profanity.x64 --matching profanity.txt --suffix-count 8 --output result.txt --result-key "your-password"

# Single address fuzzy match
./profanity.x64 --matching TUqEg3dzVEJNQSVW2HY98z5X8SBdhmao8D --prefix-count 4

# Telegram daemon mode (no matching file required — rules set via bot)
./profanity.x64 --tg-token "TOKEN" --tg-chat "CHAT_ID"

# Telegram daemon with encrypted output
./profanity.x64 --tg-token "TOKEN" --tg-chat "CHAT_ID" --result-key "your-password" --output result.txt
```

Full parameter reference:

```
-h / --help             Show help
-m / --matching         Rule file path or 34-char address
-o / --output           Result output file
-b / --prefix-count     Number of prefix positions to match (max 10)
-e / --suffix-count     Number of suffix positions to match (max 10)  
-w / --work             OpenCL local work size (default: 64)
-n / --no-cache         Disable OpenCL kernel binary cache
-T / --tg-token         Telegram Bot Token (enables daemon mode)
-C / --tg-chat          Telegram Chat ID for notifications
-K / --result-key       AES-256 encryption password for result file
```

---

## Security

- **Network access is scoped exclusively to `api.telegram.org`** — no other outbound connections are made. All legacy backdoor code (remote POST endpoints, debug seeds) has been removed.
- **Private keys never appear in Telegram messages.** Notifications include the found address only.
- **Result file encryption** uses AES-256-CBC with PBKDF2 key derivation. The password is never transmitted over any network.
- **Randomness** — The original fixed-seed RNG vulnerability has been replaced with proper OS-level entropy.

---

## Project Structure

```
Trongo/
├── profanity.cpp          Main entry point, CLI parsing, bot lifecycle
├── Dispatcher.cpp/.hpp    OpenCL device management, GPU kernel dispatch
├── TGBot.cpp/.hpp         Native C++ Telegram Bot (long-polling, keyboards)
├── Mode.cpp/.hpp          Rule parsing and address matching logic
├── ArgParser.hpp          Command-line argument parser
├── SpeedSample.cpp/.hpp   Hash rate sampling
├── json.hpp               nlohmann/json (single-header, bundled)
├── kernel_profanity.hpp   Embedded OpenCL kernel (profanity)
├── kernel_keccak.hpp      Embedded OpenCL kernel (Keccak-256)
├── kernel_sha256.hpp      Embedded OpenCL kernel (SHA-256)
├── tron                   Interactive CLI manager shell script
├── profanity.txt          Default vanity rule file
└── Makefile
```

---

## License

This project is licensed under the MIT License. See `LICENSE` for details.

Original profanity engine by [johguse/profanity](https://github.com/johguse/profanity). Telegram integration, encryption, and CLI tooling by the Trongo contributors.