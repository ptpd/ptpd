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

from scapy.packet 		import *
from scapy.fields 		import *
from scapy.layers.inet 	import UDP
from scapy.all 			import * 
import struct

# Clock Identity for all messages
class ClockIdentity(BitField) :
	def __init__(self, name) :
		BitField.__init__(self, name, 0, 64)

# Timestamp for all messages
class Timestamp(Packet) :
	name = "Timestamp"
	fields_desc = [ BitField("secs",		   0,		 48),
					BitField("nanos",		   0,		 32)
				  ]

	def __add__(self, delta):
		secs  = self.secs  + delta.secs
		nanos = self.nanos + delta.nanos
		# carry
		carry = nanos / (10 ** 9)
		# now pass it on
		return Timestamp(secs = secs + carry, nanos = nanos - carry * (10 ** 9))

	def __sub__(self, delta):
		secs  = self.secs  - delta.secs
		nanos = self.nanos - delta.nanos
		# carry
		while nanos < 0 :
			secs  -= 1
			nanos += 10 ** 9
		# now pass it on
		return Timestamp(secs = secs, nanos = nanos)

#######################
# Security Extensions #
#######################

# EC Signature
class Signature(FieldListField) :
	def __init__(self, name) :
		FieldListField.__init__(self, name, 0, XByteField)

# EC Public Key
class Pubkey(FieldListField) :
	def __init__(self, name) :
		FieldListField.__init__(self, name, 0, XByteField)

# PK Certificate
class Certificate(FieldListField) :
	def __init__(self, name) :
		FieldListField.__init__(self, name, 0, XByteField)

########################
# PTP Header + Packets #
########################

# PTP Header
class PTP(Packet) :

	# message Types
	SYNC			= 0x00
	DELAY_REQ		= 0x01
	PDELAY_REQ		= 0x02
	PDELAY_RESP		= 0x03
	FOLLOW_UP		= 0x08
	DELAY_RESP		= 0x09
	PDELAY_RESP_FU	= 0x0a
	ANNOUNCE		= 0x0b
	SIGNALING		= 0x0c
	MANAGMENT		= 0x0d

	PTPMessageType	=	{SYNC:				'Sync',
						 DELAY_REQ:			'Delay_Req',
						 PDELAY_REQ:		'PDelay_Req',
						 PDELAY_RESP:		'PDelay_Resp',
						 FOLLOW_UP:			'Follow_UP',
						 DELAY_RESP:		'Delay_Resp',
						 PDELAY_RESP_FU:	'PDelay_Resp_FU',
						 ANNOUNCE:			'Announce',
						 SIGNALING:			'Signaling',
						 MANAGMENT:			'Managment'
						}

	# PTP Flags (by order)
	SECURITY 			= 1 << 15
	PROFILE_SPECIFIC_2	= 1 << 14
	PROFILE_SPECIFIC_1	= 1 << 13
	RESERVED_0			= 1 << 12
	RESERVED_1			= 1 << 11
	UNICAST				= 1 << 10
	TWO_STEP			= 1 <<  9
	ALTERNATE_MASTER	= 1 <<  8
	RESERVED_2			= 1 <<  7
	RESERVED_3			= 1 <<  6
	FREQUENCY_TRACEABLE	= 1 <<  5
	TIME_TRACEABLE		= 1 <<  4
	TIMESCALE			= 1 <<  3
	UTC_REASONABLE		= 1 <<  2
	L1_59				= 1 <<  1
	L1_61				= 1 <<  0

	PTPFlags		= [ 'Security',
						'Profile Specific 2',
						'Profile Specific 1',
						'Reserved 0',
						'Reserved 1',
						'Unicast',
						'Two Step',
						'Alternate Master',
						'Reserved 2',
						'Reserved 3',
						'Frequency Traceable',
						'Time Traceable',
						'Timescale',
						'UTC Reasonable',
						'L1 59',
						'L1 61'
					  ]
	PTPFlags.reverse()

	name = "PTP Header"
	fields_desc = [ BitField("transportSpecific",		   0,		 4),
					BitEnumField("messageType",			None,		 4,	PTPMessageType),
					BitField("reserved0",				   0,		 4),
					BitField("version",					   2,		 4),
					BitField("messageLength",			  34,		16),
					BitField("domainNumber",			   0,		 8),
					BitField("reserved1",				   0,		 8),
					FlagsField("flags",					   0,		16,	PTPFlags),
					BitField("correctionField",			   0,		64),
					BitField("reserved2",				   0,		32),
					ClockIdentity("sourceClockIdentity"),
					BitField("portIdentity",			   1,		16),
					BitField("sequenceId",				   0,		16),
					BitField("controlField",			   0,		 8),
					BitField("logMessageInterval",		   0,		 8)
				  ]

	def post_build(self, p, pay) :
		# add the payload 
		p += pay

		# set up the msg's length
		if self.messageLength is None :
			l = len(p)
			p = p[:2] + struct.pack("!h", l) + p[4:]
			
		# control fields
		c = 0x05

		# set up the msg's type
		if self.messageType is None :
			if isinstance(self.payload, PTPSync) :
				t = SYNC
				c = 0x00
			elif isinstance(self.payload, PTPDelayReq) :
				t = DELAY_REQ
				c = 0x01
			elif isinstance(self.payload, PTPPDelayReq) :
				t = PDELAY_REQ
			elif isinstance(self.payload, PTPPDelayResp) :
				t = PDELAY_RESP
			elif isinstance(self.payload, PTPFollowUp):
				t = FOLLOW_UP
				c = 0x02
			elif isinstance(self.payload, PTPDelayResp) :
				t = DELAY_RESP
				c = 0x03
			elif isinstance(self.payload, PTPPDelayRespFU) :
				t = PDELAY_RESP_FU
			elif isinstance(self.payload, PTPAnnounce):
				t = ANNOUNCE
			else :
				warning("PTP: cannot set the messageType! defaulting to SYNC (0x00)")
				t = SYNC
			# insert it to it's place
			p = chr(ord(p[0]) | t) + p[1:32] + ord(c) + p[33:]

		# return the result
		return p

# Sync Message
class PTPSync(Packet) :
	name = "PTP Sync"
	fields_desc = [ PacketField("originTimestamp", 	Timestamp(), Timestamp) ]

# Delay Request Message
class PTPDelayReq(Packet) :
	name = "PTP Delay Req"
	fields_desc = [ PacketField("originTimestamp", 	Timestamp(), Timestamp) ]

# Peer Delay Request Message
class PTPPDelayReq(Packet) :
	name = "PTP PDelay Req"
	fields_desc = [ PacketField("originTimestamp", 	Timestamp(), Timestamp),
					BitField("reserved",			0,		80)
				  ]

# Follow Up Message
class PTPFollowUp(Packet) :
	name = "PTP Follow Up"
	fields_desc = [ PacketField("preciseOriginTimestamp", Timestamp(), Timestamp),
				    ConditionalField(Signature("signature"), lambda p : p.underlayer != None and (p.underlayer.flags & PTP.SECURITY) != 0)
				  ]

# Delay Response Message
class PTPDelayResp(Packet) :
	name = "PTP Delay Resp"
	fields_desc = [ PacketField("receiveTimestamp", 			Timestamp(), Timestamp),
					ClockIdentity("requestingClockIdentity"),
					BitField("requestingPortIdentity",			1,		16)
				  ]

# Peer Delay Response Message
class PTPPDelayResp(Packet) :
	name = "PTP PDelay Resp"
	fields_desc = [ PacketField("receiveReceiptTimestamp", 		Timestamp(), Timestamp),
					ClockIdentity("requestingClockIdentity"),
					BitField("requestingPortIdentity",			1,		16)
				  ]

# Peer Delay Response Follow Up Message
class PTPPDelayRespFU(Packet) :
	name = "PTP PDelay Resp FU"
	fields_desc = [ PacketField("responseOriginTimestamp", 		Timestamp(), Timestamp),
					ClockIdentity("requestingClockIdentity"),
					BitField("requestingPortIdentity",			1,		16)
				  ]

# Announce Message
class PTPAnnounce(Packet) :
	# time source typese
	ATOMIC_CLOCK		= 0x10
	GPS					= 0x20
	TERRESTRIAL_RADIO	= 0x30
	PTP					= 0x40
	NTP					= 0x50
	HAND_SET			= 0x60
	OTHER				= 0x90
	INTERNAL_OSCILLATOR	= 0xA0

	TimeSources =	{ ATOMIC_CLOCK:			'Atomic Clock',
					  GPS:					'GPS',
					  TERRESTRIAL_RADIO:	'Terrestrial Radio',
					  PTP:					'PTP',
					  NTP:					'NTP',
					  HAND_SET:				'Hand Set',
					  OTHER:				'Other',
					  INTERNAL_OSCILLATOR:	'Internal Oscillator'
					}

	name = "PTP Announce"
	fields_desc =	[ PacketField("originTimestamp", Timestamp(), 	Timestamp),
					  BitField("currentUtcOffest",       0,					16),
					  BitField("reserved",		         0,					 8),
					  BitField("priority1",		       128,					 8),
					  BitField("ClockQuality",	0x80fe7060,					32),
					  BitField("priority2",		       128,					 8),
					  ClockIdentity("identity"),
					  BitField("stepsRemoved",	         0,					16),
					  BitField("timeSource",		INTERNAL_OSCILLATOR,	 8),
					  ConditionalField(Certificate("certificate"), lambda p : p.underlayer != None and (p.underlayer.flags & PTP.SECURITY) != 0)
					]

# bind up the layers
bind_layers( Ether	, PTP,				type  = 0x88f7	)
bind_layers( UDP	, PTP,				dport = 319		) # event
bind_layers( UDP	, PTP,				dport = 320		) # general
bind_layers( PTP	, PTPSync,			messageType = PTP.SYNC				)
bind_layers( PTP	, PTPDelayReq,		messageType = PTP.DELAY_REQ			)
bind_layers( PTP	, PTPPDelayReq,		messageType = PTP.PDELAY_REQ		)
bind_layers( PTP	, PTPFollowUp,		messageType = PTP.FOLLOW_UP			)
bind_layers( PTP	, PTPDelayResp,		messageType = PTP.DELAY_RESP		)
bind_layers( PTP	, PTPPDelayResp,	messageType = PTP.PDELAY_RESP		)
bind_layers( PTP	, PTPPDelayRespFU,	messageType = PTP.PDELAY_RESP_FU	)
bind_layers( PTP	, PTPAnnounce,		messageType = PTP.ANNOUNCE			)
