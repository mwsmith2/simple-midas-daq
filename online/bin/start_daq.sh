#!/bin/bash
# The script starts general midas utilites for sis_wfd experiment.

source /home/newg2/Applications/simple-daq/common/.expt-env

source $EXPT_DIR/online/bin/start_midas.sh
source $EXPT_DIR/online/bin/start_frontends.sh
source $EXPT_DIR/online/bin/start_analyzers.sh

# end script