#!/bin/bash

if [ $# != 1 ]
then
    echo "usage: $0 <data-dir>" >&2
    exit 1
fi

data=$1
mkdir -p data

(
echo "var datadir = 'images';"
echo "var videos = ["
ls $data | sed 's/^/"/;s/$/",/'
echo "];"
) > data/videos.js

for show in `ls $data`
do
    (
    echo "images = ["
    cat $data/$show/$show.liste.txt | sed 's/^/"/;s/$/",/'
    echo "];"
    echo "sim = ["
    cat $data/$show/$show.sim | sed 's/ /,/g;s/^/[/;s/$/],/'
    echo "];"
    echo "if(callback) callback();"
    ) > data/${show}.js
    mkdir -p images/$show
done

find $data -name \*.png | while read image; do target=images/$(basename $(dirname $image))/$(basename $image);echo "echo $image;convert -resize 512x288\\! $image $target; convert -resize 192x108\\! $image ${target}.192"; done | parallel -j 4
