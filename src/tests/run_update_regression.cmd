@echo off
setlocal
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cd /d "%~dp0.."
if not exist "x64\Tests" mkdir "x64\Tests"
cl /nologo /std:c++latest /EHsc /O2 /MD /I. tests\update_regression.cpp /Fo:x64\Tests\update_regression.obj /Fe:x64\Tests\update_regression.exe /link bcrypt.lib crypt32.lib /IGNORE:4099
if errorlevel 1 exit /b 1
x64\Tests\update_regression.exe
