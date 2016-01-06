CC = gcc
SRCS_DIR = src
SRCS = $(foreach dir_var, $(SRCS_DIR), $(wildcard $(dir_var)/*.c))
OBJS = $(patsubst %.c, objs/%.o, $(SRCS))

CFLAGS = --std=c99 -g -Wall -O2

LIBS = -lpthread -lm

all:begin coix end

begin:
	@echo "compile begin"
	@mkdir -p objs/src

coix: $(OBJS)
	$(CC) -o objs/$@ $(CFLAGS) $^ $(LIBS)

objs/%.o: %.c
		$(CC) -c $(CFLAGS) -o $@ $<

end:
	@echo "compile finished"

.PHONY:clean
clean:
	-rm -rf objs/
