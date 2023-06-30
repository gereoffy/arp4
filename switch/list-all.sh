
# cd /var/arp4/switch

cat switch.conf |while read line; do ./list-snmp.py $line ; done >ports.all 2>names.new

./topology3.sh >topology.new

# lookup switch mac addresses:
for ip in $(cut -f 1 switch.conf.SAMPLE |cut -d ' ' -f 1|cut -d '@' -f 2) do arp -n $ip ; done | \
    grep -o '[0-9a-zA-Z][0-9a-zA-Z]:[0-9a-zA-Z][0-9a-zA-Z]:[0-9a-zA-Z][0-9a-zA-Z]:[0-9a-zA-Z][0-9a-zA-Z]:[0-9a-zA-Z][0-9a-zA-Z]:[0-9a-zA-Z][0-9a-zA-Z]' | \
    tr a-z A-Z >sw-macs.txt

# find the uplink ports (using sw-macs list) and remove the uplink/downlink entries:
./normalize.py <ports.all | sort >ports.new

mv -f ports.new ports.txt
mv -f names.new names.txt
mv -f topology.new topology.txt

