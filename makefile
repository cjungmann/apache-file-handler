all: simple.fcgi

simple.fcgi : simplefcgi.c
	gcc simplefcgi.c -L /user/local/lib -lfcgi -o simple.fcgi

install:
	/etc/init.d/apache2 stop
	cp simple.fcgi /usr/local/lib/cgi-bin
	/etc/init.d/apache2 start

