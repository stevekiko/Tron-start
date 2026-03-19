@echo off
chcp 65001 >nul
setlocal

set "BASE_DIR=%~dp0"
set "EXEC=%BASE_DIR%profanity.exe"
set "TG_CONFIG=%BASE_DIR%tg_config.txt"
set "RESULT_FILE=%BASE_DIR%result.txt"

if not exist "%EXEC%" (
    echo [错误] 找不到 %EXEC% 核心引擎！
    echo 请先编译项目：在 MSYS2 MINGW64 终端执行 make，或用 Visual Studio 生成 Release x64。
    pause
    goto :eof
)

if "%1"=="stop" goto :stop
if "%1"=="start" goto :start
if "%1"=="-r" goto :result
if "%1"=="result" goto :result
if "%1"=="-s" goto :speed
if "%1"=="speed" goto :speed

:usage
echo.
echo  ===  Tron-start Windows 管理器  ===
echo.
echo  用法：  trongo.bat [命令]
echo.
echo  命令列表：
echo    start   启动 Telegram Bot 守护进程（推荐，通过 TG 控制一切）
echo    stop    停止所有运行中的引擎进程
echo    -r      查看爆号结果文件
echo    -s      查看算力文件
echo.
echo  提示：Windows 版以 Telegram Bot 为主要交互方式。
echo        启动后打开 Telegram，通过底部按键菜单远程操控。
echo.
pause
goto :eof

:: ======== start ========
:start
tasklist /fi "imagename eq profanity.exe" 2>nul | find "profanity.exe" >nul
if not errorlevel 1 (
    echo [警告] 引擎已在后台运行！请先执行 trongo.bat stop。
    pause
    goto :eof
)

set TG_TOKEN=
set TG_CHAT_ID=
set RESULT_KEY=

if exist "%TG_CONFIG%" (
    for /f "usebackq tokens=1,* delims==" %%a in ("%TG_CONFIG%") do (
        if "%%a"=="TG_TOKEN"    set TG_TOKEN=%%~b
        if "%%a"=="TG_CHAT_ID"  set TG_CHAT_ID=%%~b
        if "%%a"=="RESULT_KEY"  set RESULT_KEY=%%~b
    )
)

echo.
echo  === Tron-start Telegram Bot 配置向导 ===
echo.

if not "%TG_TOKEN%"=="" (
    echo  已读取到历史配置 Token: %TG_TOKEN:~0,12%...  Chat: %TG_CHAT_ID%
    set /p REUSE= 沿用此配置？(Y/n，默认 Y)：
    if /i "%REUSE%"=="n" (
        set TG_TOKEN=
        set TG_CHAT_ID=
    )
)

if "%TG_TOKEN%"=="" (
    set /p TG_TOKEN= 请输入 Bot Token（由 BotFather 获取）：
    set /p TG_CHAT_ID= 请输入接收通知的 Chat ID：
    (
        echo TG_TOKEN=!TG_TOKEN!
        echo TG_CHAT_ID=!TG_CHAT_ID!
    ) > "%TG_CONFIG%"
)

if "%RESULT_KEY%"=="" (
    echo.
    echo  [可选] 私钥文件加密（需要系统已安装 openssl 命令）
    set /p RESULT_KEY= 设置加密密码（直接回车跳过）：
    if not "%RESULT_KEY%"=="" (
        echo RESULT_KEY=!RESULT_KEY! >> "%TG_CONFIG%"
        echo  [OK] 加密密码已设置！
    )
)

echo.
echo  正在后台启动 Telegram Bot 守护进程...

if not "%RESULT_KEY%"=="" (
    start /B "" "%EXEC%" --tg-token "%TG_TOKEN%" --tg-chat "%TG_CHAT_ID%" --result-key "%RESULT_KEY%" --output "%RESULT_FILE%"
) else (
    start /B "" "%EXEC%" --tg-token "%TG_TOKEN%" --tg-chat "%TG_CHAT_ID%"
)

timeout /t 2 >nul
tasklist /fi "imagename eq profanity.exe" 2>nul | find "profanity.exe" >nul
if not errorlevel 1 (
    echo  [OK] 启动成功！打开你的 Telegram，用底部按钮远程控制吧。
) else (
    echo  [错误] 启动失败！请检查 Token 格式或防火墙设置。
)
pause
goto :eof

:: ======== stop ========
:stop
echo  正在停止所有 Tron-start 引擎进程...
taskkill /f /im profanity.exe >nul 2>&1
if not errorlevel 1 (
    echo  [OK] 已停止。
) else (
    echo  [提示] 未发现运行中的进程。
)
pause
goto :eof

:: ======== result ========
:result
if exist "%RESULT_FILE%" (
    echo  === 最近爆号结果 ===
    more "%RESULT_FILE%"
) else (
    echo  暂无结果，让程序再跑一会。
)
pause
goto :eof

:: ======== speed ========
:speed
set SPEED_FILE=%BASE_DIR%speed.txt
if exist "%SPEED_FILE%" (
    type "%SPEED_FILE%"
) else (
    echo  暂无速度数据，程序可能尚未发车。
)
pause
goto :eof
