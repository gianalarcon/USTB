all:server client clean
#使用的编译器
CC = gcc
#编译server.c文件
server:server.o
	$(CC) -lpthread -g server.c -o server
server.o:server.c my_head.h
	$(CC) -lpthread -c -g server.c my_head.h
#编译client.c文件
client:client.o
	$(CC) -lpthread -g client.c -o client
client.o:client.c my_head.h
	$(CC) -lpthread -c -g client.c my_head.h
#清除.o文件
clean:
	-rm server.o client.o