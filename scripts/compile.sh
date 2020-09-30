#!/bin/bash

GCC=/usr/bin/gcc

# If we have arm processor, we are on beaglebone; compile for beaglebone
# Else, we are on lnxsrv; compile for lnxsrv
uname -a | grep "armv7l"
if [ $? -eq 0 ]
then
	$GCC -g -lmraa -lm -o ./tcp ./src/tcp.c
	$GCC -g -lmraa -lm -lssl -lcrypto -o ./tls ./src/tls.c
	$GCC -g -lmraa -lm -lssl -lcrypto -o ./local_prog ./src/local_prog.c
else
	$GCC -g -lm -o ./tcp -DDUMMY ./src/tcp.c 
	$GCC -g -lm -lssl -lcrypto -o ./tls -DDUMMY ./src/tls.c
	$GCC -g -lm  -o ./local_prog -DDUMMY ./src/local_prog.c
fi
