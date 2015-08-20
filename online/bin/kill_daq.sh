#!/bin/bash
# The script starts general midas utilites for the experiment.

source $(dirname $(readlink -f $0))/../../common/.expt-env

source $EXPT_DIR/online/bin/kill_analyzers.sh
source $EXPT_DIR/online/bin/kill_frontends.sh
source $EXPT_DIR/online/bin/kill_midas.sh

# end script