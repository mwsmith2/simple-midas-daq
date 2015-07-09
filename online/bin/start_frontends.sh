#!/bin/bash

# Load the experiment variables.
source /home/newg2/Applications/simple-daq/common/.expt-env

for fe in "${EXPT_FE[@]}"; do
    fename="${EXPT_DIR}/online/frontends/bin/${fe}"
    scname="${EXPT}.${fe//_/-}"
    screen -dmS $scname
    screen -S $scname -p 0 -rX stuff "${fename} -e $EXPT$(printf \\r)"
done

# end script
