
all: logger

logger: main.o logger.o atomic.o log_time.o common.o
	gcc main.o logger.o atomic.o log_time.o common.o

main.o: main.c
	gcc -c main.c

logger.o: logger.c
	gcc -c logger.c

atomic.o: atomic.c
	gcc -c atomic.c

log_time.o: log_time.c
	gcc -c log_time.c

common.o: common.c
	gcc -c common.c


clean:
	rm -rf *.o logger
