# NAME:   Zachary Berger
# EMAIL:  zackeberger@gmail.com
# ID:     705082271 

# Prevent sanity check from failing: -Wall -Wextra

build: compile.sh lab4c_tcp.c lab4c_tls.c
	bash compile.sh

clean:
	rm -rf lab4c_tcp lab4c_tls lab4c-705082271.tar.gz

dist: lab4c_tcp.c lab4c_tls.c compile.sh Makefile README
	tar -zcvf lab4c-705082271.tar.gz lab4c_tcp.c lab4c_tls.c compile.sh Makefile README
