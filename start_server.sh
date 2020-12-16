#!/bin/bash
if [ $# -ne 1 ]
then
	echo "Provide server ID as argument."
	exit 1
fi

myid=$1

mkdir /local/$USER
mkdir /local/$USER/SETIData
touch /local/$USER/SETIData/myid
echo "$myid" > /local/$USER/SETIData/myid


./worker