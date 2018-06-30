crntl: crntl.o main.o
	$(CC) -o crntl crntl.o main.o
crntl.o: crntl.c crntl.h
main.o: main.c crntl.h
clean:
	rm -fv *.o crntl
