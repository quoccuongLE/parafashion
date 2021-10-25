
# Install instructions

## CMakeLists project

```
git submodule update --init --recursive
mkdir build
cd build
cmake ..
make -j parafashion
./parafashion
```

## From Nico's Parafashion

First, uncomment `LIBS += -lGLULIBS += -lGLU` in `Parafashion.pro`.

From the project folder:

```
sudo apt install qt5-default libqt5svg5-dev freeglut3-dev

# Compile AntTweakBar
cd lib/AntTweakBar1.16/src
make
cd ../../..

# temporary solution, set your own path here
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/corentin/Parafashion/lib/AntTweakBar/lib/
```

Then compile and run:
```
qmake Parafashion
make -j parafashion && ./parafashion test_data/leggins/leggins.ply test_data/leggins/leggins.ply 
```