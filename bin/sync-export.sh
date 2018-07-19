#!/bin/bash
DIR_SRC=~/kiribako/export
DIR_DST=hep4.nucl.phys.titech.ac.jp:/var/www/html/kiribako/data
touch $DIR_SRC/timestamp
rsync --verbose --archive $DIR_SRC $DIR_DST
#rsync --verbose --archive --delete --dry-run $DIR_SRC $DIR_DST
