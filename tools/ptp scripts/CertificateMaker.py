#!/usr/bin/python

###############################################################################
# Copyright (c) 2015-2016 Eyal Itkin (TAU),                                   #
#                         Avishai Wool (TAU),                                 #
#                         In accordance to http://arxiv.org/abs/1603.00707 .  #
#                                                                             #
# All Rights Reserved                                                         #
#                                                                             #
# Redistribution and use in source and binary forms, with or without          #
# modification, are permitted provided that the following conditions are      #
# met:                                                                        #
# 1. Redistributions of source code must retain the above copyright notice,   #
#    this list of conditions and the following disclaimer.                    #
# 2. Redistributions in binary form must reproduce the above copyright        #
#    notice, this list of conditions and the following disclaimer in the      #
#    documentation and/or other materials provided with the distribution.     #
#                                                                             #
# THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR       #
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED              #
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE      #
# DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE         #
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR                #
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF        #
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR             #
# BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,       #
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE        #
# OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN      #
# IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.                               #
###############################################################################

from ptp import *
import sys
import os

pub_path = '.pub'
log_path = 'certificate'

##
# Gets the IP of the current machine
##
def getLocalIP(iface) :
	ip = os.popen('ifconfig ' + iface + ' | grep "inet\ addr" | cut -d: -f2 | cut -d" " -f1').read()[:-1]
	print 'Local IP is:', ip
	return ip

##
# Builds the clock_id from the given ip according to the following rules:
# net_id   = IP | UDP_EVENT_PORT (319)
# clock_id = net[:3] | 0xFF | 0xFE | net[3:]
##
def convertIPtoID(ip_list) :
	net_id = []
	# start with the network id
	net_id.append(int(ip_list[3]))
	net_id.append(int(ip_list[2]))
	net_id.append(int(ip_list[1]))
	net_id.append(int(ip_list[0]))
	net_id.append(319 / 256)
	net_id.append(319 % 256)
	# now build up the clock id itself
	clock_id = 0
	j = 0
	for i in xrange(8) :
		if i == 3 :
			candidate = 0xFF
		elif i == 4 :
			candidate = 0xFE
		else :
			candidate = net_id[j]
			j += 1
		# sum up the candidate
		clock_id = (clock_id << 8) + candidate
	# return the result
	return clock_id

##
# Usage Guide
##
def printUsage(script_name) :
	print 'Usage: %s <is_master: 1 / 0> <iface: eth1> <priority1: 64>' % (os.path.split(script_name)[-1])
	print 'Note 1: The master/slave public key file should be named: master.pub / slave.pub accordingly
	print 'Note 2: The finalized certificate will be written to the output file "certificate"
	print 'Exitting'
	sys.exit()

##
# Main certificate maker
##	
def Main(args) :
	# check arguments
	if len(args) != 1 + 3 :
		print 'Wrong amount of arguments, got %d while expected %d' % (len(args) - 1, 2)
		printUsage(args[0])

	# build up the id from ip
	clock_id = convertIPtoID(getLocalIP(args[2]).split('.'))
	# get the priority
	priority = int(args[3])

	# build up the Announce packet
	print 'Clock ID is: 0x%x' % clock_id
	packet = PTPAnnounce(identity = clock_id, priority1 = priority)

	# should read the public key
	key_path = ('master' if int(args[1]) == 1 else 'slave' ) + pub_path
	print 'Reading the key:', key_path
	pub_key  = open(key_path, 'r').read()

	# dump the packet to a file
	open(log_path, 'wb').write(str(packet) + pub_key)
	print 'Built up the Certificate'


# activate the main
if __name__ == "__main__":
	Main(sys.argv)
