#!/bin/bash
# Script to export local files to the web server.
# To be executed manually or via crontab:
#   */10 *  * * *  $HOME/kiribako/bin/sync-export.sh &>/dev/null
DIR_BASE=$(readlink -f $(dirname $0)/../)
DIR_SRC=$DIR_BASE/export
DIR_DST=hep4.nucl.phys.titech.ac.jp:/var/www/html/kiribako/data

FN_ENA=$DIR_SRC/.sync-export.enabled
FN_GRP=$DIR_SRC/.sync-export.conf
if [ "X$1" = "Xon" ] ; then
    echo "Enable the export and exit."
    touch $FN_ENA
    exit
elif [ "X$1" = "Xoff" ] ; then
    echo "Disable the export and exit."
    rm $FN_ENA
    exit
elif [ "X$1" = "Xgroup" ] ; then
    echo "Set the group name to '$2'."
    echo "$2" >$FN_GRP
    exit
elif [ "X$1" = "Xungroup" ] ; then
    echo "Remove the conf file."
    rm -f $FN_GRP
    exit
elif [ ! -e $FN_ENA ] ; then
    echo "Do nothing since not enabled."
    exit
fi

if [ -f $FN_GRP ] ; then
    echo "Read '$FN_GRP'."
    DIR_DST=$DIR_DST/$(awk 'NR==1{print $1}' $FN_GRP)
fi

touch $DIR_SRC/timestamp
rsync --verbose --archive $DIR_SRC $DIR_DST
#rsync --verbose --archive --delete --dry-run $DIR_SRC $DIR_DST
