#!/bin/sh
mkdir temp
cd temp
cp ../*.ppd ./
cp ../*.sh ./
./1_rename.sh
./2_delete_lang.sh
./3_remove_model_vendor.sh
ppdi -o epson_inkjet_org.drv ./*.ppd
cp ./epson_inkjet_org.drv ../../epson_inkjet_org.drv
rm -rf ./temp/
cd ..
