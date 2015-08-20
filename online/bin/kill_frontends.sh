#!/bin/bash

# The script starts ends midas frontends for the experiment.
source $(dirname $(readlink -f $0))/../../common/.expt-env

for fe in "${EXPT_FE[@]}"; do
    for session in $(screen -ls | grep -o "[0-9]*\.${EXPT}.${fe//_/-}"); do
        screen -S "${session}" -p 0 -X quit
    done
done

# end script