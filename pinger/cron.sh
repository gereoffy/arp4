#! /bin/bash

TZ='Europe/Budapest'
export TZ

cd /var/diag
./monitor.py >change.new

N=$(grep -c 'CHANGED' <change.new)
M=$(grep -c -v 'ping=0' status.dat)
if test $N -gt 0; then
    date >>change.log
    grep 'CHANGED' <change.new >>change.log
    (echo "Subject: PING monitor - $N changes ($M down)"
    date
    echo ""
    grep 'CHANGED' <change.new
    echo ""
    grep -v 'CHANGED' <change.new | grep -v ': OK$'
    echo "" ) | /usr/sbin/sendmail log@your.email.com
fi
