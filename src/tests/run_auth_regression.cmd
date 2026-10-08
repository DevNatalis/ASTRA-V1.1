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
cl /nologo /std:c++latest /EHsc /O2 /MD /DCURL_STATICLIB /I. tests\auth_regression.cpp /Fo:x64\Tests\auth_regression.obj /Fe:x64\Tests\auth_regression.exe /link /LIBPATH:Security\Api\curl advapi32.lib /IGNORE:4099
if errorlevel 1 exit /b 1
x64\Tests\auth_regression.exe
