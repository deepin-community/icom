#!/bin/csh
set month = `date '+%m'`
set day = `date '+%d'`
set hour = `date '+%H'`
set str = `minimuf -m $month -d $day -h $hour -s $SRF -o 4 cfh-rtty.dat dipole.dat`
set stamp = `date '+%h %d %T '`
echo -n $stamp cfh $str "tuned "
icom -r r72 -m usb -o -1.75 -g $str[5] -d
