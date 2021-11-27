
cd /var/arp4/switch

cat switch.conf |while read line; do ./list-snmp.py $line ; done >ports.all 2>names.new

./topology3.sh >topology.new

#for i in $(grep -o '19[23].[.0-9]*' list-all.sh ); do arp -a -n $i ; done|cut -d ' ' -f 4 |tr a-z A-Z >sw-macs.txt

./normalize.py <ports.all | sort >ports.new

# >D253-ports.txt


#cat *-names.txt  >names.txt
#cat *-ports.txt

mv -f ports.new ports.txt
mv -f names.new names.txt
mv -f topology.new topology.txt

#/usr/bin/ssh cache 'cat >/var/arp4/switch/ports.txt' </var/arp4/switch/ports.txt
# /usr/bin/ssh cache 'cat >/var/arp4/switch/names.txt' </var/arp4/switch/names.txt
