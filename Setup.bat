@echo off

cls

::#####################################VARS#############################################################################
set SCRIPT_FOLDER=%cd%

set GRPC_ROOT=%SCRIPT_FOLDER%\grpc
set PROTOBUF_SOLUTION_FOLDER=%GRPC_ROOT%\third_party\protobuf\cmake\build\solution

set REMOTE_ORIGIN=https://github.com/grpc/grpc.git
set BRANCH=v1.3.x

set VSVARSALL="C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat"
set CMAKE_FOLDER='C:\Program Files\CMake\bin'
::#####################################VARS#############################################################################

:SET_ENV_VARS
echo Setting cmake && SET PATH=C:/Program Files/CMake/bin;%PATH%
echo Setting nuget && SET PATH=%cd%;%PATH%

:SET_VS_ENV
echo Setting vcvarsall && call %VSVARSALL% amd64

:CREATE_FOLDERS
set ARTIFACTS_ROOT=%GRPC_ROOT%\Artifacts
set LIBS_ROOT=%ARTIFACTS_ROOT%\grpc_libs
set STRIPPED_ROOT=%ARTIFACTS_ROOT%\grpc_stripped
set BIN_ROOT=%ARTIFACTS_ROOT%\grpc_bin

if exist %SCRIPT_FOLDER%\nuget.exe goto :CLONE_OR_PULL

:DOWNLOAD_NUGET
cd %SCRIPT_FOLDER%
powershell -executionpolicy bypass -Command Invoke-WebRequest https://dist.nuget.org/win-x86-commandline/latest/nuget.exe -OutFile "nuget.exe"

:CLONE_OR_PULL
if EXIST %GRPC_ROOT% (cd %GRPC_ROOT% && echo Pulling repo && git pull) else (call git clone %REMOTE_ORIGIN% && cd %GRPC_ROOT%)

if exist %ARTIFACTS_ROOT% rmdir %ARTIFACTS_ROOT% /s /q
mkdir %ARTIFACTS_ROOT%
mkdir %LIBS_ROOT%
mkdir %STRIPPED_ROOT%
mkdir %BIN_ROOT%

git fetch
git checkout -f
git checkout -t origin/%BRANCH%
git submodule update --init

:CLEAN_ALL
cd %GRPC_ROOT%
git clean -fdx
git submodule foreach git clean -fdx

:COPY_HEADERS
echo Synchronizing %STRIPPED_ROOT%
robocopy %GRPC_ROOT%\include %STRIPPED_ROOT%\include /E > nul
robocopy %GRPC_ROOT%\third_party\protobuf\src %STRIPPED_ROOT%\third_party\protobuf\src /E > nul

:GENERATE_VS_PROTOBUF_SOLUTION
cd "%GRPC_ROOT%\third_party\protobuf\cmake"
mkdir build & cd build
mkdir solution & cd solution
cmake -G "Visual Studio 14 2015 Win64" -Dprotobuf_BUILD_TESTS=OFF -Dprotobuf_WITH_ZLIB=ON ../..

:PATCH_VS_SOLUTION
cd %GRPC_ROOT%\..
powershell -executionpolicy bypass -file edit_props.ps1

:UPGRADE_VS_SOLUTIONS
cd %PROTOBUF_SOLUTION_FOLDER%
devenv protobuf.sln /upgrade

cd %GRPC_ROOT%/vsprojects
devenv grpc_protoc_plugins.sln /upgrade

:CLEAN_PROTOBUF_SOLUTION
cd %PROTOBUF_SOLUTION_FOLDER%
devenv protobuf.sln /clean "Debug|x64"
devenv protobuf.sln /clean "Release|x64"

:BUILD_PROTOBUF
cd %PROTOBUF_SOLUTION_FOLDER%
devenv protobuf.sln /build "Release|x64" /project ALL_BUILD

:BUILD_GRPC
cd "%GRPC_ROOT%\vsprojects"
nuget restore grpc.sln
devenv grpc.sln /upgrade
devenv grpc.sln /clean "Release|x64"
devenv grpc.sln /build "Release|x64" /project grpc++
devenv grpc.sln /build "Release|x64" /project grpc++_unsecure

:BUILD_PLUGINS
cd "%GRPC_ROOT%\vsprojects"
nuget restore grpc_protoc_plugins.sln
devenv grpc_protoc_plugins.sln /upgrade
devenv grpc_protoc_plugins.sln /clean "Release|x64"
devenv grpc_protoc_plugins.sln /build "Release|x64"

:Finish
cd %GRPC_ROOT%
echo Build done!

:COPY_LIBS
echo Synchronizing %LIBS_ROOT%
robocopy "%GRPC_ROOT%\vsprojects\packages\grpc.dependencies.zlib.1.2.8.10\build\native\lib\v140\x64\Release\static\stdcall" %LIBS_ROOT% *.lib /R:0 /S > nul
robocopy "%GRPC_ROOT%\vsprojects\packages\grpc.dependencies.openssl.1.0.204.1\build\native\lib\v140\x64\Release\static" %LIBS_ROOT% *.lib /R:0 /S > nul
robocopy "%GRPC_ROOT%\third_party\protobuf\cmake\build\solution\Release" %LIBS_ROOT% "libprotobuf.lib" > nul
robocopy "%GRPC_ROOT%\third_party\protobuf\cmake\build\solution\Release" %LIBS_ROOT% "libprotobuf-lite.lib" > nul
robocopy "%GRPC_ROOT%\vsprojects\x64\Release" %LIBS_ROOT% *.lib /R:0 /S > nul

:COPY_BIN
echo Synchronizing %BIN_ROOT%
robocopy "%GRPC_ROOT%\third_party\protobuf\cmake\build\solution\Release" %BIN_ROOT% "protoc.exe" > nul
robocopy "%GRPC_ROOT%\vsprojects\x64\Release" %BIN_ROOT% *.exe /R:0 /S > nul
