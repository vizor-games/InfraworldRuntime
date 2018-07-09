@echo off

cls

::#####################################VARS#############################################################################
set SCRIPT_FOLDER=%cd%

set GRPC_ROOT=%SCRIPT_FOLDER%\grpc
set PROTOBUF_SOLUTION_FOLDER=%GRPC_ROOT%\third_party\protobuf\cmake\build\solution

set REMOTE_ORIGIN=https://github.com/grpc/grpc.git
set BRANCH=v1.3.x

set VSVARSALL="C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat"
set DEVENV_PATH="C:\Program Files (x86)\Microsoft Visual Studio 14.0\Common7\IDE\"
set CMAKE_FOLDER='C:\Program Files\CMake\bin'
::#####################################VARS#############################################################################

:SET_ENV_VARS
echo ================Setting EVIRIOMENT VARS================
echo Setting cmake && SET PATH=C:/Program Files/CMake/bin;%PATH%
echo Setting nuget && SET PATH=%cd%;%PATH%
echo Setting robocopy && SET PATH=C:\Windows\System32;%PATH%
echo ================-DONE- Setting EVIRIOMENT VARS -DONE-================

:SET_VS_ENV
echo ================Setting VS VARS================
echo Setting vcvarsall && call %VSVARSALL% amd64
echo Setting devenv && call SET PATH=%DEVENV_PATH%;%PATH%
echo ================-DONE- Setting VS VARS -DONE-================

:CREATE_FOLDERS
echo ================Creating Folders================
set LIBS_ROOT=%SCRIPT_FOLDER%\GrpcLibraries
set STRIPPED_ROOT=%SCRIPT_FOLDER%\GrpcIncludes
set BIN_ROOT=%SCRIPT_FOLDER%\GrpcPrograms
echo ================-DONE- Creating Folders -DONE-================

if exist %SCRIPT_FOLDER%\nuget.exe goto :CLONE_OR_PULL

:DOWNLOAD_NUGET
echo ================Downloading Nuget================
cd %SCRIPT_FOLDER%
powershell -executionpolicy bypass -Command Invoke-WebRequest https://dist.nuget.org/win-x86-commandline/latest/nuget.exe -OutFile "nuget.exe"
echo ================-DONE- Downloading Nuget -DONE-================

:CLONE_OR_PULL
echo ================CLONE GRPC================
if EXIST %GRPC_ROOT% (cd %GRPC_ROOT% && echo Pulling repo && git pull) else (call git clone %REMOTE_ORIGIN% && cd %GRPC_ROOT%)

mkdir %LIBS_ROOT%
mkdir %STRIPPED_ROOT%
mkdir %BIN_ROOT%

git fetch
git checkout -f
git checkout -t origin/%BRANCH%
git submodule update --init
echo ================-DONE- CLONE GRPC -DONE-================

:CLEAN_ALL
echo ================GIT CLEAN ALL================
cd %GRPC_ROOT%
git clean -fdx
git submodule foreach git clean -fdx
echo ================-DONE- GIT CLEAN ALL -DONE-================

:COPY_HEADERS
echo ================SYNCHRONIZING %STRIPPED_ROOT%================
robocopy %GRPC_ROOT%\include %STRIPPED_ROOT%\include /E > nul
robocopy %GRPC_ROOT%\third_party\protobuf\src %STRIPPED_ROOT%\third_party\protobuf\src /E > nul
echo =============== -DONE- Synchronizing %STRIPPED_ROOT% -DONE-================

:GENERATE_VS_PROTOBUF_SOLUTION
echo ================Generating VS Protobuf Solution================
cd "%GRPC_ROOT%\third_party\protobuf\cmake"
mkdir build & cd build
mkdir solution & cd solution
cmake -G "Visual Studio 15 2017 Win64" -Dprotobuf_BUILD_TESTS=OFF -Dprotobuf_WITH_ZLIB=ON ../..
echo ================ -DONE- Generating VS Protobuf Solution -DONE-=============

:PATCH_VS_SOLUTION
echo ================PATCHING VS SOLUTIONS================
cd %GRPC_ROOT%\..
powershell -executionpolicy bypass -file edit_props.ps1
echo ================-DONE- PATCHING VS SOLUTIONS -DONE-================

:UPGRADE_VS_SOLUTIONS
echo ================UPGRADING VS SOLUTIONS================
cd %PROTOBUF_SOLUTION_FOLDER%
devenv protobuf.sln /upgrade
cd %GRPC_ROOT%/vsprojects
devenv grpc_protoc_plugins.sln /upgrade
echo ================-DONE- PATCHING VS SOLUTIONS -DONE-================

:CLEAN_PROTOBUF_SOLUTION
echo ================CLEANING PROTOBUF SOLUTIONS================
cd %PROTOBUF_SOLUTION_FOLDER%
devenv protobuf.sln /clean "Debug|x64"
devenv protobuf.sln /clean "Release|x64"
echo ================-DONE- CLEANING PROTOBUF SOLUTIONS -DONE-================

:BUILD_PROTOBUF
echo ================BUILDING PROTOBUF================
cd %PROTOBUF_SOLUTION_FOLDER%
devenv protobuf.sln /build "Release|x64" /project ALL_BUILD
echo ================-DONE- BUILDING PROTOBUF -DONE-================

:RESTORE_AND_UPGRADE_GRPC
echo ================RESTORING AND UPGRADING GRPC SOLUTION================
cd "%GRPC_ROOT%\vsprojects"
nuget restore grpc.sln
devenv grpc.sln /upgrade
echo ================-DONE- RESTORING AND UPGRADING GRPC SOLUTION -DONE-================

:CLEAN_GRPC
echo ================CLEANING GRPC================
devenv grpc.sln /clean "Release|x64"
echo ================-DONE- CLEANING GRPC -DONE-================

:BUILD_GRPC
echo ================BUILDING GRPC================
devenv grpc.sln /build "Release|x64" /project grpc++
devenv grpc.sln /build "Release|x64" /project grpc++_unsecure
echo ================-DONE- BUILDING GRPC -DONE-================

:CLEAN_PLUGINS
echo ================CLEANING PLUGINS================
cd "%GRPC_ROOT%\vsprojects"
nuget restore grpc_protoc_plugins.sln
devenv grpc_protoc_plugins.sln /upgrade
devenv grpc_protoc_plugins.sln /clean "Release|x64"
echo ================-DONE- CLEANING PLUGINS -DONE-================

:BUILD_PLUGINS
echo ================BUILDING PLUGINS================
devenv grpc_protoc_plugins.sln /build "Release|x64"
echo ================-DONE- BUILDING PLUGINS -DONE-================

:COPY_LIBS
echo ================SYNCHRONIZING %LIBS_ROOT%================
robocopy "%GRPC_ROOT%\vsprojects\packages\grpc.dependencies.zlib.1.2.8.10\build\native\lib\v140\x64\Release\static\stdcall" %LIBS_ROOT%\Win64 *.lib /R:0 /S > nul
robocopy "%GRPC_ROOT%\vsprojects\packages\grpc.dependencies.openssl.1.0.204.1\build\native\lib\v140\x64\Release\static" %LIBS_ROOT%\Win64 *.lib /R:0 /S > nul
robocopy "%GRPC_ROOT%\third_party\protobuf\cmake\build\solution\Release" %LIBS_ROOT%\Win64 "libprotobuf.lib" > nul
robocopy "%GRPC_ROOT%\third_party\protobuf\cmake\build\solution\Release" %LIBS_ROOT%\Win64 "libprotobuf-lite.lib" > nul
robocopy "%GRPC_ROOT%\vsprojects\x64\Release" %LIBS_ROOT%\Win64 *.lib /R:0 /S > nul
echo ================-DONE- SYNCHRONIZING %LIBS_ROOT% -DONE-================

:COPY_BIN
echo ================SYNCHRONIZING %BIN_ROOT%================
robocopy "%GRPC_ROOT%\third_party\protobuf\cmake\build\solution\Release" %BIN_ROOT% "protoc.exe" > nul
robocopy "%GRPC_ROOT%\vsprojects\x64\Release" %BIN_ROOT% *.exe /R:0 /S > nul
echo ================-DONE- SYNCHRONIZING %BIN_ROOT% -DONE-================

:Finish
cd %GRPC_ROOT%
echo Build done!
pause
