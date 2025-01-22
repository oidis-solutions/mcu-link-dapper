#!/usr/bin/env bash
# * ********************************************************************************************************* *
# *
# * Copyright 2024 NXP
# *
# * SPDX-License-Identifier: BSD-3-Clause
# * The BSD-3-Clause license for this file can be found in the LICENSE.txt file included with this distribution
# * or at https://spdx.org/licenses/BSD-3-Clause.html#licenseText
# *
# * ********************************************************************************************************* *

libProfile="${LIB_PROFILE:-dev}"

reinstall=false
if [[ -n "$REINSTALL" && "$REINSTALL" = "true" ]]; then
    reinstall=true
fi

function logError() {
  echo "$@" >>/dev/stderr
}

function help() {
  echo "This is module builder wrapping script."
  echo "  --help          Prints help."
  echo "  --libProfile=*  Specify one of dev|eap|prod libProfile. (dev is default)"
  echo "  --reinstall     Reinstall dependencies if exists."
}

function argsParse() {
  for i in "$@"; do
    case $i in
    -h | --help)
      help
      exit 0
      ;;
    --libProfile=*)
      libProfile="${i#*=^^}" # to upper case
      shift
      ;;
    --reinstall)
      reinstall=true
      shift
      ;;
    -*)
      logError "Unknown option $i"
      help
      exit 1
      ;;
    *) ;;
    esac
  done
}

function main() {
  argsParse "$@"

  projectRoot="/var/webix/webix-dapper"

  if [[ ! -f "$projectRoot/package.json" ]]; then
    logError "Incorrect project root."
    exit 1
  fi

  cd "$projectRoot" || exit 1

  libProfile="${libProfile^^}" # to upper case
  pkgVersion=$(python3 -c "import sys, json; print(json.load(sys.stdin)['version'])" <"$projectRoot/package.json" | sed 's/\./-/g')

  rm -rf build

  echo "Starting build..."

  rm -rf cmake-build-docker
  mkdir cmake-build-docker
  cd cmake-build-docker || exit 1

  emcmake cmake ..
  cmake --build . -- -j 8 || exit 1

  echo "Build done"
}

main "$@"
