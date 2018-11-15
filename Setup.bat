@echo off

::#####################################VARS#############################################################################
set SCRIPT_FOLDER=%cd%

set GRPC_ROOT=%SCRIPT_FOLDER%\grpc

set GRPC_INCLUDE_DIR=%SCRIPT_FOLDER%\GrpcIncludes
set GRPC_LIBRARIES_DIR=%SCRIPT_FOLDER%\GrpcLibraries\Win64
set GRPC_PROGRAMS_DIR= %SCRIPT_FOLDER%\GrpcPrograms\Win64

set CMAKE_BUILD_DIR=%GRPC_ROOT%\.build

set REMOTE_ORIGIN=https://github.com/grpc/grpc.git
set BRANCH=v1.15.x

::#####################################VARS#############################################################################

:CLEAN
IF EXIST %GRPC_ROOT% (cd %GRPC_ROOT% && git clean -fdx && git submodule foreach git clean -fdx && cd %SCRIPT_FOLDER%) 
IF EXIST %GRPC_INCLUDE_DIR% (rmdir %GRPC_INCLUDE_DIR%)
IF EXIST %GRPC_LIBRARIES_DIR% (rmdir %GRPC_LIBRARIES_DIR%)
IF EXIST %GRPC_PROGRAMS_DIR% (rmdir %GRPC_PROGRAMS_DIR%)

:CLONE_OR_PULL
if EXIST %GRPC_ROOT% (cd %GRPC_ROOT% && echo Pulling repo && git pull) else (call git clone %REMOTE_ORIGIN% && cd %GRPC_ROOT%)

git fetch
git checkout -f
git checkout -t origin/%BRANCH%
git submodule update --init

:BUILD_ALL
mkdir %CMAKE_BUILD_DIR% && cd %CMAKE_BUILD_DIR%
call cmake .. -G "Visual Studio 15 2017 Win64" -DCMAKE_BUILD_TYPE=Release
call cmake --build . --target ALL_BUILD --config Release

:COPY_HEADERS
robocopy %GRPC_ROOT%\include %GRPC_INCLUDE_DIR%\include /E > nul
robocopy %GRPC_ROOT%\third_party\protobuf\src %GRPC_INCLUDE_DIR%\third_party\protobuf\src /E > nul

:COPY_LIBRARIES
robocopy "%CMAKE_BUILD_DIR%\Release" %GRPC_LIBRARIES_DIR% *.lib /R:0 /S > nul
robocopy "%CMAKE_BUILD_DIR%\third_party\boringssl\ssl\Release" %GRPC_LIBRARIES_DIR% *.lib /R:0 /S > nul
robocopy "%CMAKE_BUILD_DIR%\third_party\boringssl\crypto\Release" %GRPC_LIBRARIES_DIR% *.lib /R:0 /S > nul
robocopy "%CMAKE_BUILD_DIR%\third_party\cares\cares\lib\Release" %GRPC_LIBRARIES_DIR% *.lib /R:0 /S > nul
robocopy "%CMAKE_BUILD_DIR%\third_party\benchmark\src\Release" %GRPC_LIBRARIES_DIR% *.lib /R:0 /S > nul
robocopy "%CMAKE_BUILD_DIR%\third_party\gflags\Release" %GRPC_LIBRARIES_DIR% *.lib /R:0 /S > nul
robocopy "%CMAKE_BUILD_DIR%\third_party\protobuf\Release" %GRPC_LIBRARIES_DIR% *.lib /R:0 /S > nul
copy "%CMAKE_BUILD_DIR%\third_party\zlib\Release\zlibstatic.lib" %GRPC_LIBRARIES_DIR%\zlibstatic.lib

:COPY_PROGRAMS
robocopy "%CMAKE_BUILD_DIR%\Release" %GRPC_PROGRAMS_DIR% *.exe /R:0 /S > nul
copy "%CMAKE_BUILD_DIR%\third_party\protobuf\Release\protoc.exe" %GRPC_PROGRAMS_DIR%\protoc.exe

:Finish
cd %SCRIPT_FOLDER%
echo Build done!
