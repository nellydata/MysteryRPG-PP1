@echo off
setlocal

set "MICARE_ROOT=%~dp0"
set "MICARE_MSBUILD=C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe"

if not exist "%MICARE_MSBUILD%" (
    set "MICARE_VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
    if exist "%MICARE_VSWHERE%" (
        for /f "usebackq delims=" %%I in (`"%MICARE_VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -find MSBuild\Current\Bin\MSBuild.exe`) do set "MICARE_MSBUILD=%%I"
    )
)

if not exist "%MICARE_MSBUILD%" (
    echo ERROR: MSBuild was not found. Install Visual Studio with Desktop development with C++.
    exit /b 1
)

echo Building Micare Debug x64...
"%MICARE_MSBUILD%" "%MICARE_ROOT%COS-PP1.slnx" /t:Build /p:Configuration=Debug /p:Platform=x64 /m /v:minimal
if errorlevel 1 exit /b %errorlevel%

set "MICARE_EXE=%MICARE_ROOT%bin\x64\Debug\COS-PP1.exe"
if not exist "%MICARE_EXE%" (
    echo ERROR: Build completed but the game executable does not exist at "%MICARE_EXE%".
    exit /b 1
)

echo Starting Micare...
"%MICARE_EXE%" %*
exit /b %errorlevel%
