#!/bin/bash
DIR_BASE=$(readlink -f $(dirname $0)/../)

if [ $(readlink -f $PWD) != $DIR_BASE ] ; then
    echo "Run this script in $DIR_BASE.  Abort."
    exit
fi
if [ ! -d export ] ; then
    echo "The directory 'export' does not exist.  Abort."
    exit
fi

echo "Will move files as groups listed below:"
for FN in kiribako-*.png ; do
    echo "  File : $FN" >/dev/stderr
    echo ${FN%--*.png}
done | sort -u | while read GRP ; do
    echo "  Group: $GRP" >/dev/stderr
    DIR_GRP=export/$GRP
    mkdir -p $DIR_GRP
    mv $GRP--*.png $DIR_GRP
    ( cd export && zip -urq $GRP.zip $GRP )
done
