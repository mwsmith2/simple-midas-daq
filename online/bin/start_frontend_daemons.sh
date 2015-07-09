#!/bin/bash

# Load our experiment environment variables
source /home/newg2/Applications/simple-daq/common/.expt-env

odbedit -e $EXPT -c 'clean'
for fe in "${EXPT_FE[@]}"; do
    ${EXP_DIR}/online/frontends/bin/${fe} -e $EXPT -D
done

# end script