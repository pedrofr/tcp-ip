#!/usr/bin/sh

dev='enp63s0'

type=`basename $0`

tc qdisc del dev $dev root
#tc qdisc del dev $dev ingress
#tc qdisc del dev ifb0 root
iptables -t filter -F INPUT
iptables -t filter -F OUTPUT
iptables -t mangle -F PREROUTING
iptables -t mangle -F OUTPUT


if [ "$type" != "netserver" ];
then
 if [ "$type" != "netclient" ];
 then
  echo "exit";
  exit;
 fi
fi

if [ "$type" = "netserver" ];
then
    iptables -t mangle -A OUTPUT --ipv4 -d 10.1.8.0/24 -p tcp --sport 8000:8800 -j MARK --set-mark 0x02020202
    iptables -t mangle -A OUTPUT --ipv4 -d 10.1.8.0/24 -p udp --sport 8000:8800 -j MARK --set-mark 0x02020202
    #iptables -t mangle -A OUTPUT --ipv4 -s 10.1.8.0/24  -j MARK --set-mark 0x02020202
else
    iptables -t mangle -A OUTPUT --ipv4 -d 10.1.8.0/24 -p tcp --dport 8000:8800 -j MARK --set-mark 0x02020202
    iptables -t mangle -A OUTPUT --ipv4 -d 10.1.8.0/24 -p udp --dport 8000:8800 -j MARK --set-mark 0x02020202
    #iptables -t mangle -A OUTPUT --ipv4 -d 10.1.8.0/24  -j MARK --set-mark 0x02020202
fi
tc qdisc add dev $dev root handle 1: htb
tc class add dev $dev parent 1: classid 1:1 htb rate 100mbit




tc filter add dev $dev parent 1: protocol ip prio 1 u32 match mark 0x02020202 0xffffffff flowid 1:1

if [ "$type" = "netserver" ];
then
    tc qdisc add dev $dev parent 1:1 handle 10: netem delay 100ms 75ms reorder 25% 50% corrupt 0.5% loss 2% 25% 
    #tc qdisc add dev $dev parent 1:1 handle 10: netem delay 100ms 50ms reorder 25% 50% corrupt 0.1% loss 0.5% 25% 
    #tc qdisc add dev $dev parent 1:1 handle 10: netem delay 50ms
else
 if [ "$type" = "netclient" ];
 then
     tc qdisc add dev $dev parent 1:1 handle 10: netem delay 75ms 75ms reorder 30% 50% corrupt 0.5% loss 2% 25% 
     #tc qdisc add dev $dev parent 1:1 handle 10: netem delay 75ms 75ms reorder 30% 50% corrupt 0.1% loss 0.5% 25% 
     #tc qdisc add dev $dev parent 1:1 handle 10: netem delay 100ms
 else
   echo "ERR"
 fi
fi





