
Login name: talrn

Name & ID: Tal Randi - 315633503

Exercise Name: EX4 – Char Server

Files:

chatserver.c 	   :	 This c file implements the chat server that allow to multiple clients to communicate and send messages each other.
	
README 		   :	 This file

Remarks: 

This program implements an chat server, that gets port number as an argument, and open this port to clients so they can communicate.
Onces the server runs - it's open to many different client to access and connect with it.
While two or more clients are connected, they can send messages, and these messages will be shown to the other clients.
Each messeage will display in this format : "guest <number> <text>" - while "number" is the file descriptor that given to 
this specific client, and "text" is the body of his message.




There are three private functions:

clean_memory - this function gets the queue and clean it's memory when SIGINT signal are called.
program_closing - this function handle the signal SIGINT and call to clean memory function.
insert_message - this function gets the queue and insert to it's tail new node - new message.



