#!/bin/bash

# NAME:   Zachary Berger
# EMAIL:  zackeberger@gmail.com
# ID:     705082271 

# If we have arm processor, we are on beaglebone; compile for beaglebone
# Else, we are on lnxsrv; compile for lnxsrv
uname -a | grep "armv7l"
if [ $? -eq 0 ]
then
	/usr/bin/gcc -Wall -Wextra -g -lmraa -lm -o lab4c_tcp lab4c_tcp.c
	/usr/bin/gcc -Wall -Wextra -g -lmraa -lm -lssl -lcrypto -o lab4c_tls lab4c_tls.c
else
	/usr/local/cs/bin/gcc -Wall -Wextra -g -lm -o lab4c_tcp -DDUMMY lab4c_tcp.c 
	/usr/local/cs/bin/gcc -Wall -Wextra -g -lm -lssl -lcrypto -o lab4c_tls -DDUMMY lab4c_tls.c
fi
