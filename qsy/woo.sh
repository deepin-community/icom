#!/bin/csh
set month = `date '+%m'`
set day = `date '+%d'`
set hour = `date '+%H'`
set str = `minimuf -f 14 -m $month -d $day -h $hour -s $SRF -o 4 woo.dat dipole.dat`
set stamp = `date '+%h %d %T '`
echo -n $stamp woo $str "tuned "
icom -r r72 -m lsb -o 2.19 -g $str[5] -d
