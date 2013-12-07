#!/bin/bash

if [ $# != 2 ]
then
    echo "usage: $0 <shots-dir> <hyp-dir>" >&2
    exit 1
fi

for i in "$1"/*.shots
do
    show=`basename $i .shots`
    cat $i | awk -vshow=$show '{printf "%s %.2f %.2f shot shot_%d %s\n", show, $4, $5, NR, $0}' > "$2"/$show.hyp
done
