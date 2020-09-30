build: scripts/compile.sh src/tcp.c src/tls.c src/local_prog.c
	bash scripts/compile.sh

check: build scripts/local_test.sh
	bash scripts/local_test.sh

clean:
	rm -rf local_prog tcp tls
