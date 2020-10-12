#! /bin/bash
REMOTE_PATH=/home/alessio/brpc_journal_v3.0/src/

for i in $@
do
	if [ $i = $1 ]
	then
		file_name=$1
	else
		host_name=$i
		scp ./"$file_name" alessio@"$host_name":"$REMOTE_PATH"
	fi
done
