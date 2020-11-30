#!/bin/sh

SCRIPT=`realpath $0`
SCRIPT_PATH=`dirname ${SCRIPT}`
PLUGIN_PATH=`realpath ${SCRIPT_PATH}/../build/kvm_janus/libkvm_janus.so`

rm plugins/*.so
ln -s ${PLUGIN_PATH} plugins
janus -F . -P ./plugins
