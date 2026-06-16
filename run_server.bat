@echo off
cd /d "%~dp0"
kvstore.exe 6379 8 1000
pause
