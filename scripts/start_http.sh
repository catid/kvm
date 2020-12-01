#!/usr/bin/env bash

# Exit on first error.
set -e

# Echo commands to stdout.
set -x

# Treat undefined environment variables as errors.
set -u

python3 start_http.py
