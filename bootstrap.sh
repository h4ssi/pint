#!/bin/bash

set -u
set -e

ulimit -s unlimited

git clone -l -s -b 0.1.0 . bootstrap
cd bootstrap
make pint
cd ..
mv bootstrap/pint .
rm -rf bootstrap
