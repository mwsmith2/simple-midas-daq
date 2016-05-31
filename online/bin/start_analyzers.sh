#!/bin/bash
# Load the experiment variables.

source $(dirname $(readlink -f $0))/../../common/.expt-env

for an in "${EXPT_AN[@]}"; do
    anname="${EXPT_DIR}/common/bin/${an}"
    scname="${EXPT}.${an//_/-}"
    cmd="${anname} -e $EXPT $(printf \\r)"
    screen -dmS $scname
    screen -S $scname -p 0 -rX stuff "$cmd"
done

unset anname
unset scname

# end script
