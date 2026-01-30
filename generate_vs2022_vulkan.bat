@echo off
echo Configuring xmake with Vulkan backend...
xmake f --rhi_backend=vulkan

echo.
echo Generating Visual Studio 2022 project...
xmake project -k vsxmake2022

echo.
echo Done!
pause