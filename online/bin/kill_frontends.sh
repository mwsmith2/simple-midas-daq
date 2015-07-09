#!/bin/bash

source /home/newg2/Applications/simple-daq/common/.expt-env

for fe in "${EXPT_FE[@]}"; do
    for session in $(screen -ls | grep -o "[0-9]*\.${EXPT}.${fe//_/-}"); do
        screen -S "${session}" -p 0 -X quit
    done
done

# end script