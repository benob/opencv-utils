#!/bin/bash

data=/run/media/favre/0786-4536/shots_20130705/
mkdir -p data

(
echo "var datadir = '$data';"
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
done
