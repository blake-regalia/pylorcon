###############################################
# tx.py - copy of tx.c from the LORCON library
# This is meant as a bit of a demo showing how
#   to use pylorcon
# By: Tom Wambold <tom5760@gmail.com>
###############################################

import sys
import getopt
import time
import pylorcon

def usage():
    ustr = "Usage: "+sys.argv[0]+" [options]\n"
    ustr += "\t-h                 prints this information\n"
    ustr += "\t-i <interface>     specify the interface name\n"
    ustr += "\t-d <driver>        string indicating driver used on interface\n"
    ustr += "\t-c <channel>       channel to transmit on\n"
    ustr += "\t===optional arguments===\n"
    ustr += "\t-n <numpkts>       number of packets to send, leave blank for infinite\n"
	ustr += "\t-s <sleep>         sleep time in SECONDS between packets, leave blank for none\n"
    ustr += "\nSupported Drivers:\n"
    # The first driver is "nodriver", skip it
    for d in pylorcon.cardlist()[1:]:
        ustr += "\t"+d["name"]+" - "+d["description"]+"\n"
    return ustr
# end usage

def main():

	# WEP encrypted packet
	packet = "\x08\x41\x0a\x00\x00\x03\x1b\xc2\x45\x33\x00\x1b\x4b\x29\x61\xb1"
	packet += "\xff\x10\x07\x00\x12\x53\x00\x00\x00\x00\x00\x00\x00\x00\x3e"
	packet += "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
	packet += "\x06\x00\x00\x00\x75\x41\x37\x5a\x4b\xbc\x55\x69\x07\x58\x4c"
	packet += "\x03\xf4\xa7\x69\xbc\xdf\x46\x27\x4d\xd0\xb6\xcc\x7c\x8b\x8b"
	packet += "\x46\x06\x30\x72\x67\x72\x5d\x49\xe6\x0a\xfb\x74\xef\x59\x1c"
	packet += "\x24\x0b\x07\x60\xee\x1b\x87\xf1\x6f\x3a\x88\x54\x25\x5a\x90"
	packet += "\xb4\x68"

	try:
		opts, args = getopt.getopt(sys.argv[1:], "hi:d:c:n:s:", [])
	except getopt.GetoptError:
		print usage()
		return

	iface = driver = numpkts = None
	channel = sleep = 0

	for o, a in opts:
		if o == "-h":
			print usage()
			return
		if o == "-i":
			iface = a
		if o == "-d":
			driver = a
		if o == "-c":
			channel = int(a)
		if o == "-n":
			numpkts = int(a)
		if o == "-s":
			sleep = float(a)
	
	if iface == None:
		print "Must specify an interface name."
		return

	if driver == None:
		print "Driver name not recognized."
		return

	tx = pylorcon.Lorcon(iface, driver)
	tx.setfunctionalmode("INJECT")
	tx.setchannel(channel)
	
	try:
		tx.setmodulation("DSSS")
	except pylorcon.LorconError:
		pass

	try:
		tx.settxrate(2)
	except pylorcon.LorconError:
		pass

	if numpkts == None:
		loopfunction = lambda x: True
	else:
		loopfunction = lambda x: x < numpkts
	
	count = 0
	try:
        while loopfunction(count):
            lorcon.txpacket("Hello World!")
            count += 1
			if sleep > 0:
				time.sleep(sleep)
    except KeyboardInterrupt:
        pass

    print "%s packets transmitted" % count
    return
