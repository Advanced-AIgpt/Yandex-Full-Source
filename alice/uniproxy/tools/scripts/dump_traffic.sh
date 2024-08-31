#!/bin/bash
# Dump traffic on uniproxy.alice.yandex.net L7 balancer

HOSTNAME=$1
DUR=${2:-60}

if [[ $HOSTNAME == "" ]]; then
    echo "Dump traffic on uniproxy.alice.yandex.net L7 balancer
    Usage: $0 <hostname> [seconds]" >&2
    exit -1
fi

echo "Dump traffic on $HOSTNAME (ip6tnl0 & tun0 interfaces) for $DUR seconds..." >&2
ssh $HOSTNAME "
    tcpdump -i ip6tnl0 -w dump-ip6tnl0.pcap &
    tcpdump -i tun0 -w dump-tun0.pcap &
    sleep $DUR
    kill \`pidof tcpdump\`
"

echo "Download dumps and secrets..." >&2
ssh $HOSTNAME "cat dump-ip6tnl0.pcap && rm dump-ip6tnl0.pcap" > dump-ip6tnl0.pcap
ssh $HOSTNAME "cat dump-tun0.pcap && rm dump-tun0.pcap" > dump-tun0.pcap
ssh $HOSTNAME "cat current-uniproxy.alice.yandex.net-secrets_log-balancer-443" > secrets.log
ssh $HOSTNAME "cat /dev/shm/balancer/priv/uniproxy.alice.yandex.net.pem" > uniproxy.alice.yandex.net.pem

echo "Merge dumps..." >&2
mergecap -w merged.pcap dump-ip6tnl0.pcap dump-tun0.pcap
