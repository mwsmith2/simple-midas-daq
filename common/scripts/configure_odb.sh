#!/bin/bash

source /home/cenpa/Applications/uw-nmr-daq/resources/.expt-env

# Set up the odb
if [ ! -e $EXPT_DIR/online/logs ]; then
    mkdir $EXPT_DIR/online/logs
fi

if [ ! -e $EXPT_DIR/resources/data ]; then
    ln -s /data/midas/$EXPT $EXPT_DIR/resources/data
fi

if [ ! -e $EXPT_DIR/online/history ]; then
    mkdir $EXPT_DIR/online/history
fi

if [ ! -e $EXPT_DIR/online/elog ]; then
    mkdir $EXPT_DIR/online/elog
fi

set_cmds=(
"set \"/Logger/Message file\" \"${EXPT_DIR}/online/logs/midas.log\""
"set \"/Logger/Data Dir\" \"${EXPT_DIR}/online/data\""
"set \"/Logger/History/FILE/History dir\" \"/${EXPT_DIR}/online/history\""
"create STRING \"/Logger/Elog dir\""
"set \"/Logger/Elog dir\" \"${EXPT_DIR}/online/elog\""
"create STRING \"/Logger/ODB dump file\""
"set \"/Logger/ODB dump file\" \"${EXPT_DIR}/online/history/run%05d.xml\""
"create BOOL \"/Logger/ODB dump\""
"set \"/Experiment/MAX_EVENT_SIZE\" 0x2000000"
"set \"/Logger/ODB dump\" \"y\""
"set \"/Logger/Channels/0/Settings/Filename\" \"run%05dsub%03d.mid.gz\""
"set \"/Logger/Channels/0/Settings/Subrun byte limit\" \"1000000000\""
"set \"/Logger/Channels/0/Settings/Compression\" 1"
"set \"/Logger/Channels/0/Settings/ODB Dump\" \"y\""
"set \"/Programs/Logger/Required\" y"
"set \"/Programs/Logger/Start command\" \"mlogger -D\""
)

for cmd in "${set_cmds[@]}"; do
    echo "$cmd"
    odbedit -e $EXPT -c "$cmd"
done

#end script