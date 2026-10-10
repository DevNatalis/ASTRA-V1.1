@echo off
setlocal
rem Toolchain: reuse an active VS env (CI sets it via msvc-dev-cmd); otherwise
rem locate vcvars64 via vswhere with fallback to well-known install paths.
where cl >nul 2>nul
if not errorlevel 1 goto :have_toolchain
set "VSVARS="
if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" (
  for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2^>nul`) do set "VSINSTALL=%%i"
)
if defined VSINSTALL if exist "%VSINSTALL%\VC\Auxiliary\Build\vcvars64.bat" set "VSVARS=%VSINSTALL%\VC\Auxiliary\Build\vcvars64.bat"
if not defined VSVARS for %%E in (BuildTools Enterprise Professional Community) do (
  if not defined VSVARS if exist "C:\Program Files (x86)\Microsoft Visual Studio\2022\%%E\VC\Auxiliary\Build\vcvars64.bat" set "VSVARS=C:\Program Files (x86)\Microsoft Visual Studio\2022\%%E\VC\Auxiliary\Build\vcvars64.bat"
)
if not defined VSVARS for %%E in (Enterprise Professional Community) do (
  if not defined VSVARS if exist "C:\Program Files\Microsoft Visual Studio\2022\%%E\VC\Auxiliary\Build\vcvars64.bat" set "VSVARS=C:\Program Files\Microsoft Visual Studio\2022\%%E\VC\Auxiliary\Build\vcvars64.bat"
)
if not defined VSVARS exit /b 1
call "%VSVARS%" >nul
if errorlevel 1 exit /b 1
:have_toolchain
cd /d "%~dp0.."
if not exist "x64\Tests" mkdir "x64\Tests"
rem ImGui objs: prefer the fresh MSBuild output; legacy svchost\ dir is a local fallback.
set "IMGOBJ=svchost\x64\Release"
if exist "x64\Release\imgui.obj" set "IMGOBJ=x64\Release"
rem D3DX SDK: probe the real layout (env June 2010 vs NuGet), repo-root
rem packages (CI) first when env gives nothing usable, src-local last.
rem Falls back to "." so the flags stay valid no-ops relying on vcvars paths.
set "DXSDK_INC="
set "DXSDK_LIB="
if defined DXSDK_DIR if exist "%DXSDK_DIR%release\lib\x64\d3dx11.lib" (
  set "DXSDK_INC=%DXSDK_DIR%include"
  set "DXSDK_LIB=%DXSDK_DIR%release\lib\x64"
)
if defined DXSDK_DIR if not defined DXSDK_LIB if exist "%DXSDK_DIR%Lib\x64\d3dx11.lib" (
  set "DXSDK_INC=%DXSDK_DIR%Include"
  set "DXSDK_LIB=%DXSDK_DIR%Lib\x64"
)
if not defined DXSDK_LIB if exist "%~dp0..\..\packages\Microsoft.DXSDK.D3DX.9.29.952.8\build\native\release\lib\x64\d3dx11.lib" (
  set "DXSDK_INC=%~dp0..\..\packages\Microsoft.DXSDK.D3DX.9.29.952.8\build\native\include"
  set "DXSDK_LIB=%~dp0..\..\packages\Microsoft.DXSDK.D3DX.9.29.952.8\build\native\release\lib\x64"
)
if not defined DXSDK_LIB if exist "%~dp0..\packages\Microsoft.DXSDK.D3DX.9.29.952.8\build\native\release\lib\x64\d3dx11.lib" (
  set "DXSDK_INC=%~dp0..\packages\Microsoft.DXSDK.D3DX.9.29.952.8\build\native\include"
  set "DXSDK_LIB=%~dp0..\packages\Microsoft.DXSDK.D3DX.9.29.952.8\build\native\release\lib\x64"
)
if not defined DXSDK_INC set "DXSDK_INC=."
if not defined DXSDK_LIB set "DXSDK_LIB=."
cl /nologo /std:c++latest /EHsc /O2 /MD /DCURL_STATICLIB /I. /IIncludes /I"%DXSDK_INC%" tests\login_ui_regression.cpp /Fo:x64\Tests\login_ui_regression.obj /Fe:x64\Tests\login_ui_regression.exe /link %IMGOBJ%\imgui.obj %IMGOBJ%\imgui_draw.obj %IMGOBJ%\imgui_tables.obj %IMGOBJ%\imgui_widgets.obj %IMGOBJ%\imgui_edited.obj %IMGOBJ%\imgui_freetype.obj %IMGOBJ%\imgui_impl_dx11.obj /LIBPATH:Security\Api\curl /LIBPATH:"%DXSDK_LIB%" Includes\ImGui\Files\FreeType\win64\freetype.lib d3dx11.lib advapi32.lib d3dcompiler.lib /IGNORE:4099 /OPT:REF
if errorlevel 1 exit /b 1
x64\Tests\login_ui_regression.exe
