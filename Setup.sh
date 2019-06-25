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

DEPS=(git automake autoconf libtool make strip go pkg-config)

# Linux needs an existing UE installation
UE_ROOT=${UE_ROOT:-"/var/lib/jenkins/UE_4.20.2-release"}

if [ ! -d "$UE_ROOT" ]; then
    echo "UE_ROOT directory ${UE_ROOT} does not exist, please set correct UE_ROOT"
    exit 1
fi;

UE_PREREQUISITES="${UE_ROOT}/Engine/Extras/ThirdPartyNotUE/SDKs/HostLinux/Linux_x64/v13_clang-7.0.1-centos7/x86_64-unknown-linux-gnu"
###############################################################################

OPENSSL_LIB="${UE_ROOT}/Engine/Source/ThirdParty/OpenSSL/1_0_2h/lib/Linux/x86_64-unknown-linux-gnu"
OPENSSL_INCLUDE="${UE_ROOT}/Engine/Source/ThirdParty/OpenSSL/1_0_2h/include/Linux/x86_64-unknown-linux-gnu"

echo "SCRIPT_DIR=${SCRIPT_DIR}"
echo "GRPC_ROOT=${GRPC_ROOT}"

# Check if all tools are installed
for i in ${DEPS[@]}; do
    if [ ! "$(which ${i})" ];then
       echo "${i} not found, install via 'apt-get install ${i}'" && exit 1
    fi
done

# Check if ran under Linux
if [ $(uname) != 'Linux' ]; then
    echo "Can not work under $(uname) operating system, should be Linux! Exiting..."
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

# Compute arch string using uname
UNAME_MACH=$(echo $(uname -m) | tr '[:upper:]' '[:lower:]')
UNAME_OS=$(echo $(uname) | tr '[:upper:]' '[:lower:]')
UNAME_ARCH="${UNAME_MACH}-unknown-${UNAME_OS}-gnu"

LIBCXX_UE_DIR="${UE_ROOT}/Engine/Source/ThirdParty/Linux/LibCxx/include"
LIBC_UE_DIR="${UE_ROOT}/Engine/Source/ThirdParty/Linux/LibCxx/include"

export CC="${UE_PREREQUISITES}/bin/clang"
export CC_FOR_BUILD=${CC}
export CXX="${UE_PREREQUISITES}/bin/clang++"
export CXX_FOR_BUILD=${CXX}

# we need this to avoid 'unknow flavor: old-gnu' error
if [ ! -e "${UE_PREREQUISITES}/bin/lld-gnu" ]; then
    ln -s "${UE_PREREQUISITES}/bin/ld.lld" "${UE_PREREQUISITES}/bin/lld-gnu"
fi

find "${UE_PREREQUISITES}/usr/lib64" -name '*.o' -exec cp -vfs '{}' "${UE_PREREQUISITES}/lib64" ";"

# this thing avoid us from gcc usage, we don't need it
export VALID_CONFIG_gcov=0

# force compile protobuf, libz and libares
export HAS_SYSTEM_CARES=false
export HAS_SYSTEM_PROTOBUF=false
export HAS_SYSTEM_ZLIB=false

# funny, but in grpc Makefile LD and LDXX associated with compilers
export LD="${CC}"
export LDXX="${CXX}"

export DEFAULT_CC="${CC}"
export DEFAULT_CXX="${CXX}"

export CFLAGS="-fPIC -Wno-error --sysroot=${UE_PREREQUISITES}"
export CFLAGS_FOR_BUILD=${CFLAGS}
export CXXFLAGS="-std=c++14 -fPIC -nostdinc++ -Wno-expansion-to-defined -Wno-error -I${LIBCXX_UE_DIR} -I${LIBCXX_UE_DIR}/c++/v1 -I${OPENSSL_INCLUDE}"
export CXXFLAGS_FOR_BUILD=${CXXFLAGS}

export LIBRARY_PATH="${UE_PREREQUISITES}/usr/lib64"

export LDFLAGS="-L${UE_ROOT}/Engine/Source/ThirdParty/Linux/LibCxx/lib/Linux/${UNAME_ARCH} -L${OPENSSL_LIB} -fuse-ld=${UE_PREREQUISITES}/bin/lld-gnu"
export LDFLAGS_FOR_BUILD=${LDFLAGS}

export LDLIBS="-lc++ -lc++abi -lc"

export PROTOBUF_LDFLAGS_EXTRA="${LDFLAGS} ${LDLIBS}"

# Create an alias 'clocale -> xlocale.h' (if does not exist)
if [ ! -e "${LIBCXX_UE_DIR}/c++/v1/xlocale.h" ]; then
    if [ ! -e "${LIBCXX_UE_DIR}/c++/v1/clocale" ]; then
        echo "${LIBCXX_UE_DIR}/c++/v1/clocale must exist in UE src dir. Exiting..." && exit 1
    fi

    (cd "${LIBCXX_UE_DIR}/c++/v1" && ln -s clocale xlocale.h)
    echo "Created an alias to xlocale.h"
fi

echo "CFLAGS=${CFLAGS}, CXXFLAGS=${CXXFLAGS}, LDFLAGS=${LDFLAGS}, LDLIBS=${LDLIBS}, PROTOBUF_LDFLAGS_EXTRA=${PROTOBUF_LDFLAGS_EXTRA}"

# Build GRPC
(cd $GRPC_ROOT && make CC=${CC} CXX=${CXX})

# Copy artifacts
LIBS_DIR="${SCRIPT_DIR}/GrpcLibraries"
BIN_DIR="${SCRIPT_DIR}/GrpcPrograms"

echo "LIBS_DIR is ${LIBS_DIR}"
echo "BIN_DIR is ${BIN_DIR}"

if [ $(uname) != 'Darwin' ]; then
    ARCH_LIBS_DIR="${LIBS_DIR}/"$(uname)
    ARCH_BIN_DIR="${BIN_DIR}/"$(uname)
else
    ARCH_LIBS_DIR="${LIBS_DIR}/Mac"
    ARCH_BIN_DIR="${BIN_DIR}/Mac"
fi

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

SRC_LIBS_FOLDER_GRPC=$GRPC_ROOT/libs/opt
SRC_LIBS_FOLDER_PROTOBUF=$PROTOBUF_ROOT/src/.libs

# Force recursively copy
if [ -d "$SRC_LIBS_FOLDER_PROTOBUF" ]; then
    echo "Copying protobuf libraries from ${SRC_LIBS_FOLDER_PROTOBUF} to ${ARCH_LIBS_DIR}"
    (cd $SRC_LIBS_FOLDER_PROTOBUF && find . -name '*.a' -exec cp -vf '{}' $ARCH_LIBS_DIR ";")
fi

if [ -d "$SRC_LIBS_FOLDER_GRPC" ]; then
    echo "Copying grpc libraries from ${SRC_LIBS_FOLDER_GRPC} to ${ARCH_LIBS_DIR}"
    (cd $SRC_LIBS_FOLDER_GRPC && find . -name '*.a' -exec cp -vf '{}' $ARCH_LIBS_DIR ";")
fi

# Strip all symbols from libraries
(cd $ARCH_LIBS_DIR && strip -S *.a)

# Copy binaries (plugins & protoc)
echo "Copying executables to ${ARCH_BIN_DIR}"
(cp -a "${GRPC_ROOT}/bins/opt/." $ARCH_BIN_DIR)
(cp -a "${GRPC_ROOT}/bins/opt/protobuf/." $ARCH_BIN_DIR)

# This seems to be a hack, should modify (cp -a "${GRPC_ROOT}/bins/opt/." $BIN_DIR) to copy only files, bot dirs
(cd $ARCH_BIN_DIR && rm -rf protobuf)

#
# Build go support
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

#
# Run go build
(cd "${GOPROTO_DIR}/protoc-gen-go" && go build)
(cp "${GOPROTO_DIR}/protoc-gen-go/protoc-gen-go" $ARCH_BIN_DIR)

# Strip binaries (programs)
(cd $ARCH_BIN_DIR && strip -S *)

# Finnaly, clean all stuff
rm "${LIBCXX_UE_DIR}/c++/v1/xlocale.h"
rm "${UE_PREREQUISITES}/bin/lld-gnu"
find "${UE_PREREQUISITES}/lib64" -name '*.o' -type f -delete

# Copy source
echo 'BUILD DONE!'
