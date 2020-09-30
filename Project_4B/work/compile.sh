#!/bin/bash

# NAME:   Zachary Berger
# EMAIL:  zackeberger@gmail.com
# ID:     705082271 

# If we have arm processor, we are on beaglebone; compile for beaglebone
# Else, we are on lnxsrv; compile for lnxsrv
uname -a | grep "armv7l"
if [ $? -eq 0 ]
then
	/usr/bin/gcc -Wall -Wextra -g -lmraa -lm -o lab4b lab4b.c
else
	/usr/local/cs/bin/gcc -Wall -Wextra -g -lm -o lab4b -DDUMMY lab4b.c
fi
