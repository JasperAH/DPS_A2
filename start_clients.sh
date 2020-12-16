#!/bin/bash
if [ $# -lt 3 ];
then
	echo "Provide the number of clients per client-server, the hostname of a worker/master and the hostnames of the client-servers"
	exit 1
fi

n_clients=$1
m_hostname=$2
for srv in "${@:2}"
do
	for i in {0..$n_clients}
	do
		ssh $USER@$srv "sh -c 'nohup /home/ddps2008/DPS_A2/client $i $m_hostname > /dev/null 2>&1 &'"
	done
done


