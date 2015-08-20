#!/bin/bash
# The script starts general midas utilites for the experiment.

source $(dirname $(readlink -f $0))/../../common/.expt-env

odbedit -e $EXPT -c clean
addresslist="-a $EXPT_IP -a localhost"

for addr in $EXT_IP; do
    addresslist="$addresslist -a $addr"
done

# Restart the daq components for the basic_vme setup.
for mu in "${MIDAS_UTIL[@]}"; do

    case $mu in
        'mserver')
        cmd="mserver $addresslist -p $MSERVER_PORT$(printf \\r)"
        screen -dmS "${EXPT}.mserver"
        screen -S "${EXPT}.mserver" -p 0 -rX stuff "$cmd";;
    
        'mhttpd')
        cmd="mhttpd  -e $EXPT $addresslist --mg $MHTTPD_PORT$(printf \\r)"
        screen -dmS "${EXPT}.mhttpd"
        screen -S "${EXPT}.mhttpd" -p 0 -rX stuff "$cmd";;
    
        'mlogger')
        cmd="mlogger -e $EXPT$(printf \\r)"
        screen -dmS "${EXPT}.mlogger"
        screen -S "${EXPT}.mlogger" -p 0 -rX stuff "$cmd";;
    esac
done

unset cmd
unset addresslist
# end script
