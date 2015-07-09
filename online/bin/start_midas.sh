#!/bin/bash
# The script starts general midas utilites for simple-daq.

source /home/newg2/Applications/simple-daq/common/.expt-env

odbedit -e $EXPT -c clean
addresslist="-a $EXPT_IP -a localhost"

for addr in $EXT_IP; do
    addresslist="$addresslist -a $addr"
done

# Restart the daq components for the basic_vme setup.
cmd="mserver $addresslist -p $MSERVER_PORT$(printf \\r)"
screen -dmS "${EXPT}.mserver"
screen -S "${EXPT}.mserver" -p 0 -rX stuff "$cmd"

cmd="mhttpd  -e $EXPT $addresslist -p $MHTTPD_PORT$(printf \\r)"
screen -dmS "${EXPT}.mhttpd"
screen -S "${EXPT}.mhttpd" -p 0 -rX stuff "$cmd"

cmd="mlogger -e $EXPT$(printf \\r)"
screen -dmS "${EXPT}.mlogger"
screen -S "${EXPT}.mlogger" -p 0 -rX stuff "$cmd"

unset cmd
unset addresslist
# end script
