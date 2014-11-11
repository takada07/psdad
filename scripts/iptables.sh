iptables -A OUTPUT -p tcp --tcp-flags ALL RST,ACK -j DROP
iptables -I INPUT -p tcp --tcp-flags ALL RST,ACK -j DROP
