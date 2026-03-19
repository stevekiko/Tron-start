@echo off
chcp 936 >nul
echo 正在强制关闭所有后台运行的 Tron-start 引擎...
taskkill /f /im profanity.exe
echo.
echo [OK] 已关闭！
pause
