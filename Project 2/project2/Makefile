CFLAGS	:= -Wall -g3 -O0
LDFLAGS	:= -pthread
TGT	:= museumsim
OBJS	:= main.o log.o museumsim.o

.PHONY: all clean debug test

all: $(TGT)

clean:
	rm -rf $(OBJS) $(OBJS:.o=.d) $(TGT)

debug: all
	@./museumsim

test: all
	@./museumsim test

-include $(OBJS:.o=.d)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -MMD -o $@

$(TGT): $(OBJS)
	$(CC) $(LDFLAGS) $^ -o $@
