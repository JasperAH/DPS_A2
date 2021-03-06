#!/bin/bash
if [ $# -lt 3 ];
then
	echo "Provide the number of clients per client-server, the hostname of a worker/master and the hostnames of the client-servers"
	exit 1
fi

n_clients=$1
m_hostname=$2
for srv in "${@:3}"
do
	for i in $(eval echo "{0..${n_clients}}")
	do
		echo "$srv $i"
		ssh $USER@$srv "sh -c 'nohup /home/ddps2008/DPS_A2/client $m_hostname $i > /dev/null 2>&1 &'"
	done
done


