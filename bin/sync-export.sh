#!/bin/bash
# Script to export local files to the web server.
# To be executed manually or via crontab:
#   */10 *  * * *  $HOME/kiribako/bin/sync-export.sh &>/dev/null
DIR_SRC=~/kiribako/export
DIR_DST=hep4.nucl.phys.titech.ac.jp:/var/www/html/kiribako/data

FN_ENA=$DIR_SRC/.sync-export.enabled
if [ "X$1" = "Xon" ] ; then
    echo "Enable the export and exit."
    touch $FN_ENA
    exit
elif [ "X$1" = "Xoff" ] ; then
    echo "Disable the export and exit."
    rm $FN_ENA
    exit
elif [ ! -e $FN_ENA ] ; then
    echo "Do nothing since not enabled."
    exit
fi

FN_GRP=~/.b3exp.conf
if [ -f $FN_GRP ] ; then
    echo "Read '$FN_GRP'."
    DIR_DST=$DIR_DST/$(awk 'NR==1{print $1}' $FN_GRP)
fi

touch $DIR_SRC/timestamp
rsync --verbose --archive $DIR_SRC $DIR_DST
#rsync --verbose --archive --delete --dry-run $DIR_SRC $DIR_DST
