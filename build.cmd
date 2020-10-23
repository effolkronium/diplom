mkdir %~dp0\build
cmake %~dp0 -B"%~dp0\build" -G"Visual Studio 16 2019"
cmake  --build "%~dp0\build" --config %1
pause