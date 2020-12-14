#!/bin/bash
if [ $# == 0 ]
then
	echo "Provide server hostnames or IPs as arguments."
	exit 1
fi

myid=0

for srv in "$@"
do
	ssh $USER@$srv sh -c "nohup /home/ddps2008/DPS_A2/worker ${myid} $@ &"
	myid=$((myid + 1))
done
