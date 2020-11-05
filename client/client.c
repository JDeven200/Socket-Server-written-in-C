// Cwk2: client.c - message length headers with variable sized payloads
//  also use of readn() and writen() implemented in separate code module

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include "rdwrn.h"

#define INPUTSIZ 10

void get_uname(int socket) {

    size_t payload_length;
    struct utsname *uts;
    uts = (struct utsname *) malloc(sizeof(struct utsname));

    if (uname(uts) == -1) {
	perror("uname error");
	exit(EXIT_FAILURE);
    }

    if(readn(socket, (unsigned char *) &payload_length, sizeof(size_t)) == 0)  {
	perror("could not read size");	
	exit(EXIT_FAILURE);
    }
    if(readn(socket, (unsigned char *) uts, payload_length) == 0) {
	perror("could not read uname information");	
	exit(EXIT_FAILURE);
    }
 
    printf("Node name:    %s\n", uts->nodename);
    printf("System name:  %s\n", uts->sysname);
    printf("Release:      %s\n", uts->release);
    printf("Version:      %s\n", uts->version);
    printf("Machine:      %s\n", uts->machine);

    free(uts);

} 

void send_option(int socket, char *input)
{
    size_t payload_length = strlen(input) + 1;

        if(writen(socket, (unsigned char *) &payload_length, sizeof(size_t)) <= 0)   {
	    perror("could not write size to server");
	    exit(EXIT_FAILURE);
        }	
        if(writen(socket, (unsigned char *) input, payload_length) <= 0) {
	    perror("could not write option to server");
	    exit(EXIT_FAILURE);
        }
} 

void get_student_id(int socket) {

    char id[64];
    size_t n;

    if(readn(socket, (unsigned char *) &n, sizeof(size_t)) == 0) {
	perror("could not read size");	
	exit(EXIT_FAILURE);
    }	
    if(readn(socket, (unsigned char *) id, n) == 0) {
	perror("could not read id");	
	exit(EXIT_FAILURE);
    }

    printf("IP address followed by SID: %s\n", id);
}

void get_server_time(int socket) {

    char time[32];
    size_t n;

    if(readn(socket, (unsigned char *) &n, sizeof(size_t)) == 0) {
	perror("could not read size");	
	exit(EXIT_FAILURE);
    }	
    if(readn(socket, (unsigned char *) time, n) == 0) {
	perror("could not read server time");	
	exit(EXIT_FAILURE);
    }

    printf("Server time: %s\n", time);
} 

void get_list_of_files(int socket) {

    char list_of_files[100];
    size_t n;

    if(readn(socket, (unsigned char *) &n, sizeof(size_t)) == 0) {
	perror("could not read size");	
	exit(EXIT_FAILURE);
    }
    if(readn(socket, (unsigned char *) list_of_files, n) == 0) {
	perror("could not read file list");	
	exit(EXIT_FAILURE);
    }

    printf("List of Server files: %s\n", list_of_files);
}

void get_hello(int socket)
{
    char hello_string[32];
    size_t k;

    if(readn(socket, (unsigned char *) &k, sizeof(size_t)) == 0) {
	perror("could not read size");	
	exit(EXIT_FAILURE);
    }	
    if(readn(socket, (unsigned char *) hello_string, k) == 0) {
	perror("could not read greeting");	
	exit(EXIT_FAILURE);
    }

    printf("Hello String: %s\n", hello_string);
    printf("Received: %zu bytes\n\n", k);
}

void displaymenu()
{
    printf("0. Display menu\n");
    printf("1. Get Student ID Number\n");
    printf("2. Get Server Time\n");
    printf("3. Get Uname Information\n");
    printf("4. Get List of Server Files\n");
    printf("5. Exit Client\n");
}

int main(void)
{
    int sockfd = 0;
    char input;
    char server_input[1];
    char *svr_ipt_ptr = server_input;
    char name[INPUTSIZ];
    struct sockaddr_in serv_addr;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
	perror("Error - could not create socket");
	exit(EXIT_FAILURE);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(50001);
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) == -1)  {
	perror("Error - connect failed");
	exit(1);
    } else
       printf("Connected to server...\n");

    displaymenu();
    get_hello(sockfd);

    do {
	printf("option> ");
	fgets(name, INPUTSIZ, stdin);
	name[strcspn(name, "\n")] = 0;
	input = name[0];
	if (strlen(name) > 1)
	    input = 'x';

	switch (input) {
	case '0':
	    displaymenu();
	    break;
	case '1':
	    svr_ipt_ptr = "1";
	    send_option(sockfd, svr_ipt_ptr);
	    get_student_id(sockfd);
	    printf("\n");
	    break;
	case '2':
            svr_ipt_ptr = "2";
	    send_option(sockfd, svr_ipt_ptr);
	    get_server_time(sockfd);
	    printf("\n");
	    break;
	case '3':
	    svr_ipt_ptr = "3";
	    send_option(sockfd, svr_ipt_ptr);
	    get_uname(sockfd);
	    printf("\n");
	    break;
	case '4':
	    svr_ipt_ptr = "4";
	    send_option(sockfd, svr_ipt_ptr);
	    get_list_of_files(sockfd);
	    printf("\n");
	    break;
	case '5':
	    svr_ipt_ptr = "5";
	    send_option(sockfd, svr_ipt_ptr);
	    printf("Goodbye!\n");
	    break;
	default:
	    svr_ipt_ptr = "x";
	    send_option(sockfd, svr_ipt_ptr);
	    printf("Invalid choice - 0 displays options...!\n");
	    break;
	}
    } while (input != '5');

    close(sockfd);

    exit(EXIT_SUCCESS);
}

