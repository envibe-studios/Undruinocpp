@echo off
REM Runs PartitionGaeaCanyons.ps1 without changing system ExecutionPolicy.
setlocal
cd /d "%~dp0.."
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0PartitionGaeaCanyons.ps1" %*
exit /b %ERRORLEVEL%
