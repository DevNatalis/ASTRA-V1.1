@echo off
setlocal
title svchost - Instalador de dependencias

rem Sobe para administrador sozinho
net session >nul 2>&1
if %errorlevel% neq 0 (
    echo Pedindo permissao de administrador...
    powershell -NoProfile -ExecutionPolicy Bypass -Command "Start-Process '%~f0' -Verb RunAs"
    exit /b
)

echo ============================================
echo  svchost - instalando o necessario
echo ============================================
echo.

rem 1) Confere se o painel esta junto do instalador
if not exist "%~dp0svchost.exe" (
    echo [ERRO] svchost.exe nao encontrado nesta pasta.
    echo Extraia TODOS os arquivos do .rar para a mesma pasta e rode de novo.
    pause
    exit /b 1
)

rem 2) Visual C++ Redistributable 2022 x64
set "REGKEY=HKLM\SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\x64"
reg query "%REGKEY%" /v Version >nul 2>&1
if %errorlevel% equ 0 (
    echo [OK] Visual C++ Redistributable ja instalado. Pulando.
) else (
    echo Baixando Visual C++ Redistributable 2022 x64...
    curl -L --fail -o "%TEMP%\vc_redist.x64.exe" "https://aka.ms/vs/17/release/vc_redist.x64.exe"
    if %errorlevel% neq 0 (
        echo [ERRO] Falha no download. Confira a internet e tente de novo.
        pause
        exit /b 1
    )
    echo Instalando (janela fechada, aguarde)...
    "%TEMP%\vc_redist.x64.exe" /install /quiet /norestart
    if %errorlevel% neq 0 (
        echo [ERRO] A instalacao falhou. Rode o arquivo como administrador.
        pause
        exit /b 1
    )
    del "%TEMP%\vc_redist.x64.exe" >nul 2>&1
    echo [OK] Visual C++ Redistributable instalado.
)

rem 3) Checagem rapida dos arquivos do painel
if not exist "%~dp0Updater.exe" echo [AVISO] Updater.exe ausente: o auto-update nao vai funcionar.
if not exist "%~dp0d3dx9_43.dll" echo [AVISO] d3dx9_43.dll ausente: pode faltar DirectX June 2010.

echo.
echo ============================================
echo  Pronto! Execute svchost.exe como
echo  ADMINISTRADOR, com o FiveM instalado,
echo  e faca login com sua conta.
echo ============================================
pause
