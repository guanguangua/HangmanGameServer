# This is a template makefile, generated to bind all the objects for HW#2. 
# Use this template to replace the placeholder text with the name of your specific program.
# Please note that you can either choose to use the example given below as it is, or make 
# a custom makefile by modifying the code.
# Here is an example of how the makefile should look -
##################### Start Example ############################
# CC = gcc
# ARGS = -Wall

# all: server_c_udp client_c_udp server_c_tcp client_c_tcp

# server_c_udp: server_c_udp.c
# 	$(CC) $(ARGS) -o server_c_udp server_c_udp.c

# client_c_udp: client_c_udp.c
# 	$(CC) $(ARGS) -o client_c_udp client_c_udp.c

# server_c_tcp: server_c_tcp.c
# 	$(CC) $(ARGS) -o server_c_tcp server_c_tcp.c

# client_c_tcp: client_c_tcp.c
# 	$(CC) $(ARGS) -o client_c_tcp client_c_tcp.c

# clean:
# 	rm -f *.o server_c_udp *~
# 	rm -f *.o client_c_udp *~
# 	rm -f *.o server_c_tcp *~
# 	rm -f *.o client_c_tcp *~

##################### End Example ############################

CC = gcc
ARGS = -Wall -pthread


# Compiling all the dependencies
all: client server

# Replace <"your_program"> with the name of your specififc program. 
# For example, the next line may look something like this: 'server_c_udp: server_c_udp.c' without quotes.
server: hangman_server.c
	$(CC) $(ARGS) -o hangman_server hangman_server.c

client: hangman_client.c
	$(CC) $(ARGS) -o hangman_client hangman_client.c

clean:
	rm -f *.o hangman_server *~
	rm -f *.o hangman_client *~