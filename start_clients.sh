#!/bin/bash
if [ $# -lt 2 ];
then
	echo "Provide the number of clients per client-server and the hostnames of the client-servers"
	exit 1
fi

experiment=1
n_clients=$1
n_messages=0
n_servers=0

servers=""
if [ $experiment == 1 ]; then
	for srv in "${@:1}"
	do
		servers="${servers}${servers:+ }$srv"
	done
	n_servers=$((n_servers + 1))
else
	for srv in "${@:1}"
	do
		servers="${servers}${servers:+ }$srv"
	done
	n_servers=$((n_servers + 1))
fi

#ssh user@host "sh -c 'nohup ./my_program > /dev/null 2>&1 &'" TODO mss moeten we nog wat met die dev/null en die 2>&1 &

ids=1

for srv in $servers
do
	if [ $experiment == 1 ]; then
		ssh $USER@$srv "sh -c 'nohup ./client $n_clients'"
		ids=$((ids + 1))
	else
		ssh $USER@$srv "sh -c 'nohup ./client $n_clients'"
		ids=$((ids + 1))		
	fi		
done
