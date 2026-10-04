@echo off
setlocal
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cd /d "%~dp0.."
if not exist "x64\Tests" mkdir "x64\Tests"
cl /nologo /std:c++latest /EHsc /O2 /MD /DCURL_STATICLIB /I. /IIncludes /I"%DXSDK_DIR%Include" tests\login_ui_regression.cpp /Fo:x64\Tests\login_ui_regression.obj /Fe:x64\Tests\login_ui_regression.exe /link ASTRA\x64\Release\imgui.obj ASTRA\x64\Release\imgui_draw.obj ASTRA\x64\Release\imgui_tables.obj ASTRA\x64\Release\imgui_widgets.obj ASTRA\x64\Release\imgui_edited.obj ASTRA\x64\Release\imgui_freetype.obj ASTRA\x64\Release\imgui_impl_dx11.obj /LIBPATH:Security\Api\curl /LIBPATH:"%DXSDK_DIR%Lib\x64" Includes\ImGui\Files\FreeType\win64\freetype.lib advapi32.lib d3dcompiler.lib /IGNORE:4099 /OPT:REF
if errorlevel 1 exit /b 1
x64\Tests\login_ui_regression.exe
