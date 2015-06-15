#!/bin/bash
#
# Emanuele Faranda         black.silver@hotmail.it
#
# A script to format gcov output
#
# Usage:    gcovfmat.sh < gcov_out
#

notexec=
yesexec=
cur=
while read line; do
    x=${line/"File '"/}
    if [[ $x != $line ]]; then
        x=${line##*/}
        cur=${x%*"'"}
    else
        x=${line/"%"/}
        if [[ ( $x != $line ) && ( ! -z $cur ) ]]; then
            x=${line#*:}
            x=`printf %5.1f ${x%'%'*}`
            yesexec="${yesexec}\n${cur}#${x}%"
            cur=
        elif [[ $line == "No executable lines" ]]; then
            notexec="$notexec $cur"
            cur=
        fi
    fi
done

echo "Tested modules:"
echo -e "${yesexec#'\n'}" | column -s'#' -t | sed 's/^/\t/'
echo -n "Untested modules:"
echo -e ${notexec// /'\n\t'}
