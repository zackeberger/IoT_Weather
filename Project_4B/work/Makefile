# NAME:   Zachary Berger
# EMAIL:  zackeberger@gmail.com
# ID:     705082271 

# Prevent sanity check from failing: -Wall -Wextra

build: lab4b.c
	bash compile.sh

check: build smoke_test.sh
	bash smoke_test.sh

clean:
	rm -rf lab4b lab4b-705082271.tar.gz

dist: lab4b.c compile.sh smoke_test.sh Makefile README
	tar -zcvf lab4b-705082271.tar.gz lab4b.c compile.sh smoke_test.sh Makefile README
