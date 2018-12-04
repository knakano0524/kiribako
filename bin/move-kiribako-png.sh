#!/bin/bash
DIR_BASE=$(readlink -f $(dirname $0)/../)

echo "Will move 'kiribako-*.png'."
for FN in $DIR_BASE/iontrap-*.avi ; do
    echo $(basename ${FN%--*})
done | sort -u | while read BN ; do
    echo "  Target: $BN"
    DIR_BN=$DIR_BASE/export/$BN
    mkdir -p $DIR_BN
    mv $DIR_BASE/$BN--* $DIR_BN
    zip -ur $DIR_BN/$BN.zip $DIR_BASE/$BN
done
