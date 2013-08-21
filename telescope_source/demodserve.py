import optparse
import glob
import time
import struct

import sys,os
sys.path.append(os.path.join(sys.path[0],'..','communications'))
from robustness import robustsocket
sys.path.append(os.path.join(sys.path[0],'..','analysis'))
import datparsing
from demodulation import demodulate

usage = """%prog [options] ROOT
ROOT is the directory containing the directories whose .dat files
you want to send."""
parser = optparse.OptionParser( usage=usage )
parser.add_option( '-p', '--port', action='store', type='int',
                   dest='port', default=4578,
                   help='The port to listen for connections on' )
parser.add_option( '-r', '--revs-per-send', action='store', type='int',
                   dest='revs_per_send', default=50,
                   help='Number of demodulated revolutions to send at once' )

options,args = parser.parse_args()
root, port, revs_per_send = args[0], options.port, options.revs_per_send

socket = robustsocket.RobustSocket( port, None )

def send_demod(filename):
    raw = datparsing.read_raw(filename)
    for chunk in datparsing.iter_chunks(raw, size=revs_per_send):
        demod = demodulate(chunk)
        socket.write(''.join(["$"+struct.pack('f'*49, *row)+"*" for row in demod]))

last_sent = None
while True:
    days = glob.glob(os.path.join(root,'*'))
    days = filter( (lambda s: os.path.isdir(s)), days )
    if len(days) > 0:
        dir = days[-1]
        dats = glob.glob(os.path.join(dir,'*.dat'))
        if len(dats) > 1:
            dat = dats[-2]
            if dat != last_sent:
                print "Sending %s"%dat
                try:
                    send_demod(dat)
                    last_sent = dat
                except Exception as e:
                    print "ERROR:", e
                    print "Sleeping"
                    time.sleep(5)
            else:
                print "Already sent %s"%dat
                print "Sleeping"
                time.sleep(5)
        else:
            print "Not enough .dat files yet."
    else:
        print "No valid day folder."
