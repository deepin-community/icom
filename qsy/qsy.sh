#!/bin/csh
date
set ssn = 79 
set month = `date '+%m'`
set day = `date '+%d'`
set hour = `date '+%H'`
set str = `minimuf -m $month -d $day -h $hour -s $ssn -o 4 chu.dat dipole.dat`
echo -n chu $str "tuned "
icom -r r72 -m usb -g $str[5] -d
set str = `minimuf -m $month -d $day -h $hour -s $ssn -o 4 cfh.dat dipole.dat`
echo -n cfh $str "tuned "
icom -r r71 -m rtty -g $str[5] -d
set str = `minimuf -m $month -d $day -h $hour -s $ssn -o 4 w1aw-rtty.dat dipole.dat`
echo -n w1aw $str "tuned "
icom -r 761 -m rtty -g $str[5] -d
