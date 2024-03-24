all: domaci
build: domaci.c
	gcc -c domaci.c
link: domaci.o
	gcc -o domaci domaci.o
clean:
	rm domaci.o domaci