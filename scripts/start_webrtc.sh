#!/usr/bin/env bash

# Exit on first error.
set -e

# Echo commands to stdout.
set -x

# Treat undefined environment variables as errors.
set -u

SCRIPT=`realpath $0`
SCRIPT_PATH=`dirname ${SCRIPT}`
PLUGIN_PATH=`realpath ${SCRIPT_PATH}/../build/kvm_janus/libkvm_janus.so`

rm -f plugins/*.so
ln -s ${PLUGIN_PATH} plugins
janus -F . -P ./plugins
