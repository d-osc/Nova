@echo off
REM Set console to UTF-8
chcp 65001 >nul
build\Release\nova.exe run test_thai.js
pause
