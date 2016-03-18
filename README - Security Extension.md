A joint work of Eyal Itkin and Avishai Wool, TAU, suggests a new security extension to the IEEE 1588 (v2) protocol. The work was suggested to the official IEEE 1588 work group, and can be found in details at: http://arxiv.org/abs/1603.00707.

During the work we implemented our proposal over the PTPd implementation, and made a list of changes to it.

Our changes are:
0. Init srand from /dev/urandom instead of MAC address (SEC_EXT_SEED_RANDOM)

1. Init clock ID from network source (SEC_EXT_BIND_CLOCK_ID_TO_NET_ID)
1.1. UDP / IP :         4 bytes IP | 2 bytes port
1.2. 802.1 (Ethernet)   6 bytes MAC address

2. Check receive clock ID against network ID (SEC_EXT_BIND_CLOCK_ID_TO_NET_ID)
2.1. Save receive MAC in 802.1 mode
2.2. Cmp net id to clock id right after header unpacking

3. Set up random sequence ID seed for request msgs (msg packing) (SEC_EXT_RANDOMIZE_SEQ_NUM)
3.1. Delay Reuqest
3.2. Peer Delay Request

4. Enlarge the sequence numbers of requests to 4 bytes (SEC_EXT_USE_RESERVE_SEQUENCE)
4.1. Used 2 bytes of reserved as the 2 MSBs of the sequence numbers
4.1.1. Packing of msgs
4.1.2. Unpacking of msgs
4.2. Sequence checks update
4.3. Data set structure update

5. Added a freshness check (window bounds check) for all master msgs in their recv
5.1. Sync
5.2. Announce

6. Added length check against the header.messageLength field (SEC_EXT_CHECK_LENGTH)

7. Minor bug fixes (reserved zeroing)

8. Crypto Extensions (SEC_EXT_CRYPTO)
