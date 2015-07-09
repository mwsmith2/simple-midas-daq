#!/bin/python
import sys
import os
from subprocess32 import call
from glob import glob
import numpy as np

# Move into the resources directory for the experiment.
os.chdir('/home/cenpa/Applications/midas/resources')

# Remove the broken ODB.
try: 
	call(['rm', '.ODB.SHM'])

except:
	pass

# Find the newest dumped odb
if len(sys.argv) > 1:

    odb = 'data/run%05i.odb' % int(sys.argv[1])

else:
    
    flist = glob('data/run*.odb')
    fidx = []

    for f in flist:
        fidx.append(int(f.split('/run')[1].split('.odb')[0]))

    odb = 'data/run%05i.odb' % fidx[np.argmax(fidx)]

call(['odbedit', '-e', 'gm2_nmr', '-s', '32000000', '-c', 'clean'])
call(['odbedit', '-e', 'gm2_nmr', '-c', 'load ' + odb])
call(['odbedit', '-e', 'gm2_nmr', '-c', '@.reset_odb'])
