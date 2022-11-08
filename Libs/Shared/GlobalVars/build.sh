gcc shr.c -fpic -shared -o libshr.so
gcc main2.c ./libshr.so -o prog2
./prog2