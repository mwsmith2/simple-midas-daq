from subprocess import call
import numpy as np

def mkdir_odb(dirname):
    cmd = ['odbedit', '-e', 'gm2_nmr', '-c']
    cmd.append('mkdir "' + dirname + '"')
    return call(cmd)

def create_odb(path, typestring, key):
    cmd = ['odbedit', '-e', 'gm2_nmr', '-c']
    cmd.append('create ' + typestring + ' "' + path + '/' + str(key) + '"')
    return call(cmd)

def set_odb(key, val):
    cmd = ['odbedit', '-e', 'gm2_nmr', '-c']
    cmd.append('set "' + key + '"' + str(val))
    return call(cmd)

def cmd_odb(cmdstring):
    cmd = ['odbedit', '-e', 'gm2_nmr', '-c']
    cmd.append(cmdstring)
    return call(cmd)

cmd_odb('rm -f "/Diag-Test/platform"')
cmd_odb('rm -f "/Diag-Test/fixed"')

nplatform_probes = 25
nfixed_probes = 4
num_freqs = 1

path = "/Diag-Test/platform"
mkdir_odb(path)
create_odb(path, "BOOL", "health[" + str(nplatform_probes) + "]")
create_odb(path, "DOUBLE", "freq[" + str(nplatform_probes * num_freqs) + "]")

for i in range(nplatform_probes):
    print i
    if (np.random.random() < 0.9):
        val = 1
    else:
        val = 0

    set_odb(path + "/health[" + str(i) + "]", val)
    
    for j in range(num_freqs):
        val = 46.5 + np.random.random()
        set_odb(path + "/freq[" + str(i*num_freqs + j) + "]", val)

path = "/Diag-Test/fixed"
mkdir_odb(path)
create_odb(path, "BOOL", "health[" + str(nfixed_probes) + "]")
create_odb(path, "DOUBLE", "freq[" + str(nfixed_probes * num_freqs) + "]")

for i in range(nfixed_probes):
    print i
    if (np.random.random() < 0.9):
        val = 1
    else:
        val = 0

    set_odb(path + "/health[" + str(i) + "]", val)
    
    for j in range(num_freqs):
        val = 46.5 + np.random.random()
        set_odb(path + "/freq[" + str(i*num_freqs + j) + "]", val)
    
             
    

