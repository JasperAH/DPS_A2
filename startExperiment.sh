#!/bin/bash
if [ $# -eq 3 ];
then
	echo "Provide the number of servers, the number of the number of client-servers and the number of clients per client-server"
	exit 1
fi


n_servers=$1
n_client_servers=$2
n_clients=$3


preserve -# $((n_client_servers+n_servers)) -t 00:15:00

sleep 2
$nodes = $(preserve -llist | grep -oP "${USER}.*R\t[0-9]+\t\K(.*)")

IFS=' ' read -ra node_array <<< "$nodes"

./start_servers.sh "" #TODO pass an array with hostnames here (part of node_array)

#./write_conf_and_start_servers.sh "${@:5:$n_servers}" 

sleep 5

./start_clients.sh "" #TODO pass the number of clients per client-server and array with hostnames here



