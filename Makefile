all:
	gcc process.c -o pword -lrt
	gcc thread.c -o tword -pthread
