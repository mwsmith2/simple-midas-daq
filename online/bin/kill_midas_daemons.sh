#!/bin/bash
# The script kills general midas utilites for sis_wfd experiment.

# Load experiment variables.
source /home/newg2/Applications/simple-daq/common/.expt-env

# The script kills general midas utilies.
pkill -f "mhttpd.*$EXPT"
pkill -f "mserver.*$EXPT"
pkill -f "mlogger.*$EXPT"
