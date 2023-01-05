build:
	mpicc -o tema3 tema3.c no_error.c error.c partition.c -Wall -lm -g 

clean:
	rm -rf tema3

pack:
	zip -FSr 333CA_lavinia_raduta_tema3.zip README.md Makefile *.c *.h

.PHONY: pack clean