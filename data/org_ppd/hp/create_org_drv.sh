#!/bin/sh
mkdir temp
cd temp
cp ../*.ppd ./
cp ../*.sh ./
./1_delete_lang.sh
./2_rename.sh
./3_del_duplicate.sh
./4_remove_model_vendor.sh
./5_remove_parallel_only.sh
./6_remove_product_vendor.sh
#./7_remove_osx_keyword.sh
ppdi -o hp_org.drv ./*.ppd
cp ./hp_org.drv ../../hp_org.drv
rm -rf temp/
cd ..
