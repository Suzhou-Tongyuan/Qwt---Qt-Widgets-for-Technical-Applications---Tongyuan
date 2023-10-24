#!/bin/bash

#qwt needs gl support, on centos you can install with:
#sudo  yum  install  mesa-libGL-devel  mesa-libGLU-devel

QTDIR=/opt/Qt5.14.2/5.14.2/gcc_64/

${QTDIR}/bin/qmake qwt.pro
make
