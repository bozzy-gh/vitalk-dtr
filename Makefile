CC?=gcc
PORT=3083

all: vitalk-dtr

version.c: .git/HEAD .git/index
	echo "const char *version = \"$(shell git describe --long --tags --always --match '[0-9]\.[0-9].*')\";" > $@

vitalk-dtr: vitalk.c gpio.c gpio.h vito_dtr.c vito_dtr.h vito_io.c vito_io.h vito_parameter.c vito_parameter.h telnet.c telnet.h version.h version.c
	$(CC) -D PORT=$(PORT) -pthread -Wall -o vitalk-dtr \
	    vitalk.c gpio.c vito_dtr.c vito_io.c vito_parameter.c telnet.c version.c \
	    -li2c -lm

clean:
	rm -f vitalk-dtr version.c
