@echo off
echo Configuring xmake with DirectX 12 backend...
xmake f --rhi_backend=dx12

echo.
echo Generating Visual Studio 2022 project...
xmake project -k vsxmake2022

echo.
echo Done!
pause