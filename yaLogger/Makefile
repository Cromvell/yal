SRCS := main.c logger.c atomic.c log_time.c common.c
OBJS := $(SRCS:.c=.o)
EXEC := yaLogger

all: $(SRCS) $(EXEC)

$(EXEC): $(OBJS)
	gcc $(OBJS) -o $@

.c.o:
	gcc -c $< -o $@

clean:
	rm -rf *.o $(EXEC)
