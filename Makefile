CFLAGS	+= `mysql_config --cflags` -Wall -Wextra
LDFLAGS	+= `mysql_config --libs`

.phony: clean

all: gchc

gchc: gchc.c
	$(CC) -O2 $(CFLAGS) $(LDFLAGS) -o $@ $@.c

clean:
	rm -f gchc
