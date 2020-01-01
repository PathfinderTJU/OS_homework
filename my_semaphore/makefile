default: myipc init.c reader.c writer.c
	gcc -o init myipc.o init.c
	gcc -o reader myipc.o reader.c
	gcc -o writer myipc.o writer.c
myipc: myipc.c
	gcc -c myipc.c
