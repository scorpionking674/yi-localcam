#!/bin/sh
# YiLocal DHCP hook. Uses only the DHCP server-provided network settings.
# It does not inject public DNS servers or contact any cloud service.
case "$1" in
  deconfig)
    ifconfig "$interface" 0.0.0.0
    ;;
  bound|renew)
    NETMASK=""
    [ -n "$subnet" ] && NETMASK="netmask $subnet"
    BCAST="broadcast +"
    [ -n "$broadcast" ] && BCAST="broadcast $broadcast"
    ifconfig "$interface" "$ip" $NETMASK $BCAST

    while route del default 2>/dev/null; do :; done
    for r in $router; do
      route add default gw "$r" dev "$interface" 2>/dev/null
      echo "$r" > /tmp/yilocal_gateway
      break
    done

    : > /tmp/resolv.conf
    for d in $dns; do
      echo "nameserver $d" >> /tmp/resolv.conf
    done

    echo "$ip" > /tmp/yilocal_ip
    ;;
esac
exit 0
