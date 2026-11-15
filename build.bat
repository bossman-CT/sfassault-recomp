@echo off
rem Double-click to build SF Assault Recomp from your own disc image (see README.md).
rem Checks the tools and the disc first, and says what to install if anything is missing.
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0tools\build.ps1" %*
set code=%errorlevel%
rem Opened by double-clicking: keep the window open to read the result.
echo %cmdcmdline% | find /i "/c" >nul && pause
exit /b %code%
