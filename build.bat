@echo off
rem Builds YsRecomp on Windows with a double-click (no command line needed): runs build.ps1 with PowerShell.
rem   build.bat            -> build.ps1 -Package: build, test the package and assemble the player package
rem                           (dist\YsRecomp\YsRecomp.exe and dist\YsRecomp.zip)
rem   build.bat ARGUMENTS  -> build.ps1 with those arguments (e.g. build.bat -TestAll, build.bat -Test)
rem Needs what build.ps1 needs (Visual Studio 2022 Build Tools, Python; README.md, "Building from source").
rem Opened with a double-click, the window waits for a key at the end so the result can be read.
setlocal
cd /d "%~dp0"
if "%~1"=="" (
    powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0build.ps1" -Package
) else (
    powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0build.ps1" %*
)
set "rc=%ERRORLEVEL%"
echo.
if not "%rc%"=="0" goto failed
if "%~1"=="" echo Done: the game is in dist\YsRecomp\ and dist\YsRecomp.zip.
if not "%~1"=="" echo Done.
goto end
:failed
echo FAILED (exit code %rc%). The lines above say what went wrong.
:end
rem Double-click: cmd.exe was started for this file only, so its command line names it
echo %cmdcmdline% | findstr /i /c:"%~nx0" >nul && pause
exit /b %rc%
