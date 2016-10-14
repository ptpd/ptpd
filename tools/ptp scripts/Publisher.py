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

import sys
import shutil
from os 		import listdir
from os.path 	import isfile, join
import os

# file constants
base_path		= '../..'
config_path		= '../Security Extension Configurations'
config_file		= 'configure.ac'
config_prefix	= 'configure_'
config_suffix	= '.ac'

##
# Builds the config's name according to the configuration's type
#
# config - configuration type
##
def build_config_name(config) :
	return config_prefix + config + config_suffix

##
# Extracts the archive's type from the configuration file (.ac)
#
# config - configuration file (.ac)
##
def archive_type(config) :
	return config[len(config_prefix) : -len(config_suffix)]	
	
##
# Publishes a PTPd version according to the given configuration file (.ac)
#
# config - configuration file (.ac)
##
def publish_version(config) :
	# copy the config file to the base folder
	shutil.copyfile(os.path.join(config_path, config), os.path.join(base_path, config_file))

	# change directory inside the base folder
	os.chdir(base_path)

	# configure the folder
	os.system('make distclean > /dev/null')
	os.system('./configure > /dev/null')
	
	print 'Finished publishing version: %s' % config

##
# Usage Guide
##
def printUsage(script_name) :
	print 'Usage: %s <config type: clear/net/seq/seq_ext/enc> (nothing will publish all)' % (os.path.split(script_name)[-1])
	print 'Exitting'
	sys.exit()

##
# Main publish function
##	
def Main(args) :
	# check if we want a specific config or all of them
	if len(args) not in [1,2] :
		printUsage(args[0])

	# specific config
	if len(args) == 2 :
		# sanity check
		if args[1] not in [archive_type(f) for f in listdir(config_path) if isfile(join(config_path, f))] :
			printUsage(args[0])
		# work on it
		publish_version(build_config_name(args[1]))

	# all configs		
	else :
		# for each configuration from the folder, publish it's version
		for config_file in [f for f in listdir(config_path) if isfile(join(config_path, f))] :
			publish_version(config_file)

# activate the main
if __name__ == "__main__":
	Main(sys.argv)
