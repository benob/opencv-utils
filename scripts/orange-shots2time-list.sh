#!/bin/bash
cat $* | grep 'Shot" position' | cut -f6 -d\" | perl -ne '($a, $b, $c) = $_ =~ /PT(\d+H)?(\d+M)?(\d+\.\d+S)?/; chop $a; chop $b; chop $c; $t = $a * 3600 + $b * 60 + $c; print "$t\n"'
