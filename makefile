server : server_thread.o
	gcc -o server server_thread.o

client : client_thread.o
	gcc -o client client_thread.o

server_thread.o : server_thread.c
	gcc -c -o server_thread.o server_thread.c
  
client_thread.o : client_thread.c
	gcc -c -o client_thread.o client_thread.c
  
clean : 
	rm *.o server
	rm *.o client