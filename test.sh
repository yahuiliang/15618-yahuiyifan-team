#!/bin/bash

make clean
make

for TREE_SIZE in 25000 50000 75000 100000
do
    for PATTERN in 0 1 2 3 4 5 6
    do
        for THREAD_NUM in 1 2 4 8 16 32 64 128 256
        do
            echo "Tree_size: "$TREE_SIZE", Pattern: "$PATTERN", Thread_num: "$THREAD_NUM >> cgl_result.txt
            ./main -p $PATTERN -n $THREAD_NUM -d $TREE_SIZE | tee -a cgl_result.txt cgl_plot.txt > /dev/null
        done
    done
done