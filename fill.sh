#!/bin/bash

for i in `seq 1 128`
do
    mkdir device/folder$i
    cp ~/Documents/max_size.file device/folder$i
done
