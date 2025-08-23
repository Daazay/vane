#!/bin/bash

pushd "$(dirname "$0")/.."

ACTION="gmake"
PREMAKE="./third-party/premake/premake5"
PREMAKE_SCRIPT="./scripts/premake/premake5.lua"

$PREMAKE --file=$PREMAKE_SCRIPT $ACTION

popd