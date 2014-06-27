# Clean up SolarFlare style PTP log output by removing the labels
# so that we get a clean CSV file.  The resulting file is in a
# different column order to the one expected by the PTP tools but that
# is handled elsewhere.
s/offset: //g
s/freq-adj: //g
s/in-sync: //g
s/one-way-delay: //g
s/ \[.*\]//g
