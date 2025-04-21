#!/bin/csh
dos2unix pcr.csv x
awk -f pcr.awk x >y
