#!/bin/bash

# Exit on errors if any
set -e

###############################################################################
# Should be defined as an environment variable, will be v1.3.x otherwise
branch=${branch:-v1.15.x}
clean=${clean:-true}

VAR_GIT_BRANCH=$branch
VAR_CLEAR_REPO=$clean

REMOTE_ORIGIN="https://github.com/grpc/grpc.git"
GOSUPPORT_REMOTE_ORIGIN="https://github.com/golang/protobuf.git"

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
GRPC_FOLDER_NAME=grpc
GRPC_ROOT="${SCRIPT_DIR}/${GRPC_FOLDER_NAME}"

CMAKE_BUILD_DIR="${GRPC_ROOT}/.build"

DEPS=(git automake autoconf libtool make strip clang++ go)
###############################################################################

echo "SCRIPT_DIR=${SCRIPT_DIR}"
echo "GRPC_ROOT=${GRPC_ROOT}"

UE_ROOT=${UE_ROOT:-"/var/lib/jenkins/UE_4.20.2-release"}

if [ ! -d "$UE_ROOT" ]; then
    echo "UE_ROOT directory ${UE_ROOT} does not exist, please set correct UE_ROOT"
    exit 1
fi;

# Check if all tools are installed
for i in ${DEPS[@]}; do
    if [ ! "$(which ${i})" ];then
       echo "${i} not found, install via 'brew install ${i}'" && exit 1
    fi
done

# Check if ran under Darwin
if [ $(uname) != 'Darwin' ]; then
    echo "Can not work under $(uname) operating system, should be Darwin! Exiting..."
    exit 1
fi;

# Clone or pull
if [ ! -d "$GRPC_ROOT" ]; then
    echo "Cloning repo into ${GRPC_ROOT}"
    git clone $REMOTE_ORIGIN $GRPC_ROOT
else
    # [[ ${VAR_CLEAR_REPO} ]] && cd $GRPC_ROOT && git merge --abort || true; git clean -fdx && git checkout -f .
    echo "Pulling repo"
    (cd $GRPC_ROOT && git pull)
fi

echo "Checking out branch ${VAR_GIT_BRANCH}"
(cd $GRPC_ROOT && git fetch)
(cd $GRPC_ROOT && git checkout -f)
(cd $GRPC_ROOT && git checkout -t origin/$VAR_GIT_BRANCH || true)

# Update submodules
(cd $GRPC_ROOT && git submodule update --init)

if [ "$VAR_CLEAR_REPO" = "true" ]; then
    echo "Cleaning repo and submodules because VAR_CLEAR_REPO is set to ${VAR_CLEAR_REPO}"
    (cd $GRPC_ROOT && make clean)
    (cd $GRPC_ROOT && git clean -fdx)
    (cd $GRPC_ROOT && git submodule foreach git clean -fdx)
elif [ "$VAR_CLEAR_REPO" = "false" ]; then
    echo "Cleaning is not needed!"
else
    echo "Undefined behaviour, VAR_CLEAR_REPO is ${VAR_CLEAR_REPO}!"
    exit 1
fi

# Copy INCLUDE folders, should copy:
#   - grpc/include
#   - grpc/third_party/protobuf/src
HEADERS_DIR="${SCRIPT_DIR}/GrpcIncludes"
PROTOBUF_SRC_DIR="${HEADERS_DIR}/third_party/protobuf"

# (re)-create headers directory
if [ -d "$HEADERS_DIR" ]; then
    printf '%s\n' "Removing old $HEADERS_DIR"
    rm -rf "$HEADERS_DIR"
fi

mkdir $HEADERS_DIR
mkdir -p $PROTOBUF_SRC_DIR

cp -R "${GRPC_ROOT}/include" $HEADERS_DIR
cp -R "${GRPC_ROOT}/third_party/protobuf/src" $PROTOBUF_SRC_DIR

# Build all
if [ -d "${CMAKE_BUILD_DIR}" ]; then
    printf '%s\n' "Removing old ${CMAKE_BUILD_DIR}"
    rm -rf "${CMAKE_BUILD_DIR}"
fi
mkdir -p ${CMAKE_BUILD_DIR} && cd ${CMAKE_BUILD_DIR}
echo "BUILD STARTED!"
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CONFIGURATION_TYPES=Release -Dprotobuf_BUILD_TESTS=OFF -DgRPC_ZLIB_PROVIDER=package -DZLIB_INCLUDE_DIR=${UE_ROOT}/Engine/Source/ThirdParty/zlib/v1.2.8/include/Mac -DZLIB_LIBRARY_DEBUG=${UE_ROOT}/Engine/Source/ThirdParty/zlib/v1.2.8/lib/Mac/libz.a -DZLIB_LIBRARY_RELEASE=${UE_ROOT}/Engine/Source/ThirdParty/zlib/v1.2.8/lib/Mac/libz.a -DgRPC_SSL_PROVIDER=package -DLIB_EAY_LIBRARY_DEBUG=${UE_ROOT}/Engine/Source/ThirdParty/OpenSSL/1.0.2g/lib/Mac/Debug/libcrypto.a -DLIB_EAY_LIBRARY_RELEASE=${UE_ROOT}/Engine/Source/ThirdParty/OpenSSL/1.0.2g/lib/Mac/Release/libcrypto.a -DLIB_EAY_DEBUG=${UE_ROOT}/Engine/Source/ThirdParty/OpenSSL/1.0.2g/lib/Mac/Debug/libcrypto.a -DLIB_EAY_RELEASE=${UE_ROOT}/Engine/Source/ThirdParty/OpenSSL/1.0.2g/lib/Mac/Release/libcrypto.a -DOPENSSL_INCLUDE_DIR=${UE_ROOT}/Engine/Source/ThirdParty/OpenSSL/1.0.2g/include/Mac -DSSL_EAY_DEBUG=${UE_ROOT}/Engine/Source/ThirdParty/OpenSSL/1.0.2g/lib/Mac/Debug/libssl.a -DSSL_EAY_LIBRARY_DEBUG=${UE_ROOT}/Engine/Source/ThirdParty/OpenSSL/1.0.2g/lib/Mac/Debug/libssl.a -DSSL_EAY_LIBRARY_RELEASE=${UE_ROOT}/Engine/Source/ThirdParty/OpenSSL/1.0.2g/lib/Mac/Release/libssl.a -DSSL_EAY_RELEASE=${UE_ROOT}/Engine/Source/ThirdParty/OpenSSL/1.0.2g/lib/Mac/Release/libssl.a
make
cd ${SCRIPT_DIR}

# Copy artifacts
LIBS_DIR="${SCRIPT_DIR}/GrpcLibraries"
BIN_DIR="${SCRIPT_DIR}/GrpcPrograms"

echo "LIBS_DIR is ${LIBS_DIR}"
echo "BIN_DIR is ${BIN_DIR}"

ARCH_LIBS_DIR="${LIBS_DIR}/Mac"
ARCH_BIN_DIR="${BIN_DIR}/Mac"

echo "ARCH_LIBS_DIR is ${ARCH_LIBS_DIR}"
echo "ARCH_BIN_DIR is ${ARCH_BIN_DIR}"

# Remove old libs and binaries directories
if [ -d "$ARCH_LIBS_DIR" ]; then
    printf '%s\n' "Removing old $ARCH_LIBS_DIR"
    rm -rf "$ARCH_LIBS_DIR"
fi
if [ -d "$ARCH_BIN_DIR" ]; then
    printf '%s\n' "Removing old $ARCH_BIN_DIR"
    rm -rf "$ARCH_BIN_DIR"
fi

# Create platform-specific artifacts directory
mkdir -p $ARCH_LIBS_DIR
mkdir -p $ARCH_BIN_DIR

SRC_LIBS_FOLDER=${CMAKE_BUILD_DIR}
echo "SRC_LIBS_FOLDER=${SRC_LIBS_FOLDER}"
if [ -d "$SRC_LIBS_FOLDER" ]; then
    echo "Copying grpc libraries from ${SRC_LIBS_FOLDER} to ${ARCH_LIBS_DIR}"
    (cd $SRC_LIBS_FOLDER && find . -name '*.a' -exec cp -vf '{}' $ARCH_LIBS_DIR ";")
fi

# Strip all symbols from libraries
(cd $ARCH_LIBS_DIR && strip -S *.a)

# Copy binaries (plugins & protoc)
echo "Copying executables to ${ARCH_BIN_DIR}"
cp ${SRC_LIBS_FOLDER}/grpc_cpp_plugin ${ARCH_BIN_DIR}/
cp ${SRC_LIBS_FOLDER}/grpc_csharp_plugin ${ARCH_BIN_DIR}/
cp ${SRC_LIBS_FOLDER}/grpc_node_plugin ${ARCH_BIN_DIR}/
cp ${SRC_LIBS_FOLDER}/grpc_objective_c_plugin ${ARCH_BIN_DIR}/
cp ${SRC_LIBS_FOLDER}/grpc_php_plugin ${ARCH_BIN_DIR}/
cp ${SRC_LIBS_FOLDER}/grpc_python_plugin ${ARCH_BIN_DIR}/
cp ${SRC_LIBS_FOLDER}/grpc_ruby_plugin ${ARCH_BIN_DIR}/
cp ${SRC_LIBS_FOLDER}/third_party/protobuf/protoc ${ARCH_BIN_DIR}/

GOROOT_DIR="${GRPC_ROOT}/go_packages"
GOPROTO_DIR="${GOROOT_DIR}/src/github.com/golang/protobuf"

echo "Building golang support in ${GOPROTO_DIR}"

if [ ! -d "${GOPROTO_DIR}" ]; then
    (cd $GRPC_ROOT && git clone $GOSUPPORT_REMOTE_ORIGIN $GOPROTO_DIR)
else
    (cd $GOPROTO_DIR && git pull)
fi

# Add gopath with protobuf libs
export GOPATH=$GOROOT_DIR

# Run go build
(cd "${GOPROTO_DIR}/protoc-gen-go" && go build)
(cp "${GOPROTO_DIR}/protoc-gen-go/protoc-gen-go" $ARCH_BIN_DIR)

# Finally, strip binaries (programs)
(cd $ARCH_BIN_DIR && strip -S *)

echo 'BUILD DONE!'
