#
# Makefile
#

all: clean wp_sock.o wps.a hellow_client hellow_server

wp_sock.o:
	gcc -c wp_sock.c

wps.a:wp_sock.o
	ar -r $@ wp_sock.o

hellow_client:
	gcc -o $@ hellow_client.cc wps.a

hellow_server:
	gcc -o $@ hellow_server.cc wps.a

clean:
	rm -f hellow_client hellow_server wps.a wp_sock.o
