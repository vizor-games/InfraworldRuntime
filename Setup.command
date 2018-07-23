#!/bin/bash

# Exit on errors if any
set -e

###############################################################################
# Should be defined as an environment variable, will be v1.3.x otherwise
branch=${branch:-v1.3.x}
clean=${clean:-true}

VAR_GIT_BRANCH=$branch
VAR_CLEAR_REPO=$clean

REMOTE_ORIGIN="https://github.com/grpc/grpc.git"
GOSUPPORT_REMOTE_ORIGIN="https://github.com/golang/protobuf.git"

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
GRPC_FOLDER_NAME=grpc
GRPC_ROOT="${SCRIPT_DIR}/${GRPC_FOLDER_NAME}"

DEPS=(git automake autoconf libtool make strip clang++ go)
###############################################################################

echo "SCRIPT_DIR=${SCRIPT_DIR}"
echo "GRPC_ROOT=${GRPC_ROOT}"

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

# # Build protobuf
PROTOBUF_ROOT=$GRPC_ROOT/third_party/protobuf
echo "PROTOBUF_ROOT=${PROTOBUF_ROOT}"

(cd $PROTOBUF_ROOT && ./autogen.sh)
(cd $PROTOBUF_ROOT && ./configure --disable-shared) # --disable-shared makes protoc static
(cd $PROTOBUF_ROOT && make)

# Export vars (don't know why, but grpc's Makefile does not export therse variables)
export PATH=$PROTOBUF_ROOT/src:$PATH
export LDFLAGS="-L${PROTOBUF_ROOT}/src/.libs -lprotobuf"
export CXXFLAGS="-I${PROTOBUF_ROOT}/src"

# Build GRPC
(cd $GRPC_ROOT && make static)

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
(cp "${PROTOBUF_ROOT}/src/protoc" $ARCH_BIN_DIR)

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

# Finally, strip binaries (programs)
(cd $ARCH_BIN_DIR && strip -S *)

# Copy source
echo 'BUILD DONE!'
