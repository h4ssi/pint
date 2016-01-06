#!/bin/bash

set -u
set -e

ulimit -s unlimited

step () {
    git clone -l -s -b "$1" . bootstrap
    [[ -f pint ]] && cp pint bootstrap/
    cd bootstrap/
    make "$2"
    cd ..
    mv "bootstrap/$2" pint
    rm -rf bootstrap/
}

make clean-bootstrap

step "0.1.0" "pint"
step "0.2.0" "pint-dev"
