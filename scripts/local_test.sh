#!/bin/bash

echo "Running smoke test"
echo

echo "Test 1: program checks bad --scale argument"
./local_prog --scale=G 2> /dev/null
if [ $? -ne 1 ]
then
	echo "FAILED Test #1"
else
	echo "PASSED Test #1"
fi
echo

echo "Test 2: process standard arguments"
./local_prog --period=5 --scale=C --log=logger.txt <<-EOF
SCALE=F
PERIOD=2
SCALE=C
START
STOP
LOG me am test
BAD COMMAND
OFF
EOF

if [ $? -ne 0 ]
then
	echo "FAILED Test #2 -- return != 0"
fi

if [ ! -s logger.txt ]
then
	echo "FAILED Test #2 -- no logfile created"
else
	for c in SCALE=F SCALE=C PERIOD=2 START STOP "LOG me am test" "BAD COMMAND" OFF
	do
		grep "$c" logger.txt > /dev/null
		if [ $? -ne 0 ]
		then
			echo "FAILED Test #2 -- did not log $c command"
		fi
	done	
fi
rm -rf logger.txt
echo

echo "Finished running smoke test"
