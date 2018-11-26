#!/bin/bash

if  [-d "./_install" ];then
  rm -rf ./_install
fi

mkdir ./_install
mkdir ./_install/lib

basepath=$(cd `dirname $0`; pwd)
./configure --enable-shared=no --prefix=$basepath/_install/ --with-pic

make clean
make
make install

cp ./*.o ./_install/lib
rm ./_install/lib/cli*
rm ./_install/lib/load_gen-load_gen.o
rm ./_install/lib/*la*
