CFLAGS	+= `mysql_config --cflags`
LDFLAGS	+= `mysql_config --libs`

.phony: clean

all: gchc

gchc:
	$(CC) -O2 $(CFLAGS) $(LDFLAGS) -o $@ $@.c

clean:
	rm -f gchc
