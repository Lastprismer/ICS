gcc -c main.c
gcc -Og -c fun1.c fun2.c -fpic
gcc -shared -o libfun.so fun1.o fun2.o
gcc -o prog2 main.o ./libfun.so
./prog2