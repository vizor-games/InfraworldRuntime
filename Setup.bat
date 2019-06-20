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

:GET_UE_ROOT
IF "%UE_ROOT%" == "" (echo "UE_ROOT directory does not exist, please set correct UE_ROOT via SET UE_ROOT=<PATH_TO_UNREAL_ENGINE_FOLDER>" && GOTO ABORT)

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
call cmake .. -G "Visual Studio 15 2017 Win64" -DCMAKE_CXX_STANDARD_LIBRARIES="Crypt32.Lib User32.lib Advapi32.lib" -DCMAKE_BUILD_TYPE=Release -DCMAKE_CONFIGURATION_TYPES=Release -Dprotobuf_BUILD_TESTS=OFF -DgRPC_ZLIB_PROVIDER=package -DZLIB_INCLUDE_DIR=%UE_ROOT%\Engine\Source\ThirdParty\zlib\v1.2.8\include\Win64\VS2015 -DZLIB_LIBRARY_DEBUG=%UE_ROOT%\Engine\Source\ThirdParty\zlib\v1.2.8\lib\Win64\VS2015\Debug\zlibstatic.lib -DZLIB_LIBRARY_RELEASE=%UE_ROOT%\Engine\Source\ThirdParty\zlib\v1.2.8\lib\Win64\VS2015\Release\zlibstatic.lib -DgRPC_SSL_PROVIDER=package -DLIB_EAY_LIBRARY_DEBUG=%UE_ROOT%\Engine\Source\ThirdParty\OpenSSL\1.1.1\Lib\Win64\VS2015\Debug\libcrypto.lib -DLIB_EAY_LIBRARY_RELEASE=%UE_ROOT%\Engine\Source\ThirdParty\OpenSSL\1.1.1\Lib\Win64\VS2015\Release\libcrypto.lib -DLIB_EAY_DEBUG=%UE_ROOT%\Engine\Source\ThirdParty\OpenSSL\1.1.1\Lib\Win64\VS2015\Debug\libcrypto.lib -DLIB_EAY_RELEASE=%UE_ROOT%\Engine\Source\ThirdParty\OpenSSL\1.1.1\Lib\Win64\VS2015\Release\libcrypto.lib -DOPENSSL_INCLUDE_DIR=%UE_ROOT%\Engine\Source\ThirdParty\OpenSSL\1.1.1\include\Win64\VS2015 -DSSL_EAY_DEBUG=%UE_ROOT%\Engine\Source\ThirdParty\OpenSSL\1.1.1\Lib\Win64\VS2015\Debug\libssl.lib -DSSL_EAY_LIBRARY_DEBUG=%UE_ROOT%\Engine\Source\ThirdParty\OpenSSL\1.1.1\Lib\Win64\VS2015\Debug\libssl.lib -DSSL_EAY_LIBRARY_RELEASE=%UE_ROOT%\Engine\Source\ThirdParty\OpenSSL\1.1.1\Lib\Win64\VS2015\Release\libssl.lib -DSSL_EAY_RELEASE=%UE_ROOT%\Engine\Source\ThirdParty\OpenSSL\1.1.1\Lib\Win64\VS2015\Release\libssl.lib
call cmake --build . --target ALL_BUILD --config Release

:COPY_HEADERS
robocopy %GRPC_ROOT%\include %GRPC_INCLUDE_DIR%\include /E > nul
robocopy %GRPC_ROOT%\third_party\protobuf\src %GRPC_INCLUDE_DIR%\third_party\protobuf\src /E > nul

:PATCH_HEADERS
set GENERATED_MESSAGE_TABLE_DRIVEN_FILE=%SCRIPT_FOLDER%\third_party\protobuf\src\google\protobuf\

:COPY_LIBRARIES
robocopy "%CMAKE_BUILD_DIR%\Release" %GRPC_LIBRARIES_DIR% *.lib /R:0 /S > nul
robocopy "%CMAKE_BUILD_DIR%\third_party\cares\cares\lib\Release" %GRPC_LIBRARIES_DIR% *.lib /R:0 /S > nul
robocopy "%CMAKE_BUILD_DIR%\third_party\benchmark\src\Release" %GRPC_LIBRARIES_DIR% *.lib /R:0 /S > nul
robocopy "%CMAKE_BUILD_DIR%\third_party\gflags\Release" %GRPC_LIBRARIES_DIR% *.lib /R:0 /S > nul
robocopy "%CMAKE_BUILD_DIR%\third_party\protobuf\Release" %GRPC_LIBRARIES_DIR% *.lib /R:0 /S > nul

:COPY_PROGRAMS
robocopy "%CMAKE_BUILD_DIR%\Release" %GRPC_PROGRAMS_DIR% *.exe /R:0 /S > nul
copy "%CMAKE_BUILD_DIR%\third_party\protobuf\Release\protoc.exe" %GRPC_PROGRAMS_DIR%\protoc.exe

:REMOVE_USELESS_LIBRARIES
del %GRPC_LIBRARIES_DIR%\grpc_csharp_ext.lib
del %GRPC_LIBRARIES_DIR%\gflags_static.lib
del %GRPC_LIBRARIES_DIR%\gflags_nothreads_static.lib
del %GRPC_LIBRARIES_DIR%\benchmark.lib

:Finish
cd %SCRIPT_FOLDER%
GOTO GRACEFULEXIT

:ABORT
pause
echo Aborted...

:GRACEFULEXIT
echo Build done!