all:
	gcc -g boom2doom3.c -o boom2doom3 -lm
	./boom2doom3
debug:
	gdb boom2doom3
