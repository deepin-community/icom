#!/bin/csh
set month = `date '+%m'`
set day = `date '+%d'`
set hour = `date '+%H'`
set stamp = `date '+%h %d %T '`
echo -n $stamp bbc "tuned "
icom -r r71 -m lsb -g $1 -d
