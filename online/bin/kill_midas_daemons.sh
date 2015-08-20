#!/bin/bash
# The script kills general midas utilites for the experiment.

# Load experiment variables.
source $(dirname $(readlink -f $0))/../../common/.expt-env

# The script kills general midas utilies.
pkill -f "mhttpd.*$EXPT"
pkill -f "mserver.*$EXPT"
pkill -f "mlogger.*$EXPT"
