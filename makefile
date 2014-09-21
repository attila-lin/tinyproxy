.c.o:
	gcc -g -c $?

all: proxy

# compile proxy
proxy: multi-proxy.o confutils.o
	g++ -g -o proxy multi-proxy.o confutils.o -lpthread

clean:
	rm -f proxy
	rm -f *.o
