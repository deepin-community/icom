#!/bin/csh
set month = `date '+%m'`
set day = `date '+%d'`
set hour = `date '+%H'`
set str = `minimuf -m $month -d $day -h $hour -s $SRF -o 4 w1aw-rtty.dat dipole.dat`
set stamp = `date '+%h %d %T '`
echo -n $stamp w1aw $str "tuned "
icom -r r72 -m lsb -o 2.12 -g $str[5] -d
