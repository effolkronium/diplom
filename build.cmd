mkdir %~dp0\build
cmake %~dp0 -B"%~dp0\build" -G"Visual Studio 15 2017 Win64"
cmake  --build "%~dp0\build" --config %1
pause