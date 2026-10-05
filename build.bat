@echo off
setlocal

set "PROJECT_DIR=%~dp0"
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"

if defined VCToolsInstallDir goto Build
if not exist "%VSWHERE%" goto NoVisualStudio
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSINSTALL=%%i"
if not defined VSINSTALL goto NoVisualStudio

call "%VSINSTALL%\VC\Auxiliary\Build\vcvars64.bat"
if errorlevel 1 goto ExitPrompt
goto Build

:NoVisualStudio
echo Error: Visual Studio C++ tools not found.
echo Run this from a Developer Command Prompt or install VS Build Tools.
goto ExitPrompt

:Build
if not exist "%PROJECT_DIR%bin" mkdir "%PROJECT_DIR%bin"
if not exist "%PROJECT_DIR%obj" mkdir "%PROJECT_DIR%obj"

echo Compiling Resources...
rc /nologo /fo"%PROJECT_DIR%obj\resource.res" "%PROJECT_DIR%resource.rc"

echo Compiling C++...
 cl /nologo /O2 /EHa /MT /std:c++17 ^
  /I"%PROJECT_DIR%third_party\imgui" /I"%PROJECT_DIR%third_party\imgui\backends" ^
  /Fo"%PROJECT_DIR%obj\\" ^
  /Fe:"%PROJECT_DIR%bin\MadManager.exe" ^
  "%PROJECT_DIR%main.cpp" ^
  "%PROJECT_DIR%ModLogic.cpp" ^
  "%PROJECT_DIR%FileBrowser.cpp" ^
  "%PROJECT_DIR%Globals.cpp" ^
  "%PROJECT_DIR%ImageLoader.cpp" ^
  "%PROJECT_DIR%third_party\imgui\imgui.cpp" ^
  "%PROJECT_DIR%third_party\imgui\imgui_draw.cpp" ^
  "%PROJECT_DIR%third_party\imgui\imgui_tables.cpp" ^
  "%PROJECT_DIR%third_party\imgui\imgui_widgets.cpp" ^
  "%PROJECT_DIR%third_party\imgui\backends\imgui_impl_win32.cpp" ^
  "%PROJECT_DIR%third_party\imgui\backends\imgui_impl_dx11.cpp" ^
  "%PROJECT_DIR%obj\resource.res" ^
 /link /NOLOGO /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup user32.lib gdi32.lib d3d11.lib d3dcompiler.lib dwmapi.lib ole32.lib shell32.lib

if errorlevel 1 goto ExitPrompt

echo Build successful.
echo File generated in \bin\MadManager.exe
goto Finish

:ExitPrompt
echo Build Failed.
exit /b 1

:Finish
