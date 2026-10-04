@echo off
setlocal
if "%VCPKG_ROOT%"=="" (
  echo VCPKG_ROOT is not set.
  exit /b 1
)
call "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
cmake -B build -S . -G Ninja -DCMAKE_BUILD_TYPE=Release -DVCPKG_TARGET_TRIPLET=x64-windows-static-md -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake"
if errorlevel 1 exit /b %errorlevel%
cmake --build build
if errorlevel 1 exit /b %errorlevel%
echo Built: build\MountedCastingBridge.dll
