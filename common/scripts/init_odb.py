#!/bin/python

# A python script that inializes the appropriate odb entries

import mhelper
expname = 'simple-daq'

odb = mhelper.ODB(expname)

# Set the custom display pages
odb.mkdir('Experiment/Run Parameters')
odb.mkdir('Experiment/Edit On Start')
odb.mkdir('Custom')
odb.mkdir('Params')

# Variable that need setting only
set_keys = {}
# and variables that need keys created.
put_keys = {}

put_keys['Params/config-dir'] = odb.expdir + '/resources/config/'
put_keys['Params/html-dir'] = odb.expdir + '/online/www'
put_keys['Custom/Shim Display'] = odb.expdir + '/online/www/shim_display.html'
put_keys['Custom/MSCB Display'] = odb.expdir + '/online/www/mscb_display.html'
put_keys['Logger/Data Dir'] = odb.expdir + '/resources/data'
put_keys['Logger/History Dir'] = odb.expdir + '/resources/history'
put_keys['Logger/Log Dir'] = odb.expdir + '/resources/log'
put_keys['Logger/Elog Dir'] = odb.expdir + '/online/elog'
put_keys['Logger/Message File'] = odb.expdir + '/resources/log/midas.log'
put_keys['Experiment/Run Parameters/Comment'] = 'Default'

set_keys['Logger/ODB Dump File'] = odb.expdir + '/resources/history/run%05d.xml'
set_keys['Logger/ODB Dump'] = 'y'
set_keys['Experiment/Menu Buttons'] = 'Status, ODB, Messages, Alarms, Programs, History, Config, Help'

for key in put_keys.keys():
    k = key.split('/')
    val = put_keys[key]
    odb.create_key('/'.join(k[:-1]), 'STRING', k[-1])
    odb.set_value(key, val)

for key in set_keys.keys():
    odb.set_value(key, set_keys[key])

odb.call_cmd('ln "/Experiment/Run Parameters/Comment" "Experiment/Edit On Start/Comment"')
    


