gcc -c fun1.c fun2.c
gcc -c main.c
ar rcs libfun.a fun1.o fun2.o
ar -t libfun.a | sort
gcc -static -o prog main.o -L. -lfun
./prog