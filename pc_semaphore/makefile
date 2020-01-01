default: myipc init.c producer.c consumer.c
	gcc -o init myipc.o init.c
	gcc -o producer myipc.o producer.c
	gcc -o consumer myipc.o consumer.c
myipc: myipc.c
	gcc -c myipc.c
