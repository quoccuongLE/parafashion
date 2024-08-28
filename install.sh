#!/bin/bash

# sudo apt install qtbase5-dev qtchooser qt5-qmake qtbase5-dev-tools

# sudo apt-get install libgmm-dev libboost-dev libblas-dev libsuitesparse-dev

VENV_DIR=${1:-".venv/parafashion"}
if [ -f lock.conda.yaml ]; then
    conda env create -f lock.conda.yaml --prefix $VENV_DIR
else
    conda env create -f conda.yaml --prefix $VENV_DIR
fi
