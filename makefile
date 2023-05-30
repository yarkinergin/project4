all: pvm

pvm: pvm.c
	gcc -Wall -g -o pvm pvm.c -lpthread -lm

clean:
	rm -fr pvm *~ output*