#!/bin/bash
# The script starts general midas utilites for sis_wfd experiment.

source /home/newg2/Applications/simple-daq/common/.expt-env

# The script kills general midas utilies.
for session in $(screen -ls | grep -o "[0-9]*\.${EXPT}.mhttpd")
do screen -S "${session}" -p 0 -X quit
done

for session in $(screen -ls | grep -o "[0-9]*\.${EXPT}.mserver")
do screen -S "${session}" -p 0 -X quit
done

for session in $(screen -ls | grep -o "[0-9]*\.${EXPT}.mlogger")
do screen -S "${session}" -p 0 -X quit
done

# end script

