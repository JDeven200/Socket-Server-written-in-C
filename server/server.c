// Cwk2: server.c - multi-threaded server using readn() and writen()

#include <sys/socket.h>
#include <sys/utsname.h>
#include <netinet/in.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include "rdwrn.h"


void *client_handler(void *);

void send_student_id(int);

void send_hello(int);

char *get_ip_address();

void get_and_send_server_time(int);

char *get_option(int);

void get_and_send_uname(int);

void stat_file(char *);

void list_files(int);

struct timeval t1, t2;

static void sig_handler(int sig, siginfo_t *siginfo, void *context)
{
    if(gettimeofday(&t2, NULL) == -1) { 
	perror("gettimeofday error");
	exit(EXIT_FAILURE);
    }

    printf("Total execution time = %f seconds\n",(double) (t2.tv_usec - t1.tv_usec) / 1000000 + (double) (t2.tv_sec - t1.tv_sec));

    printf("Server shutting down...\n");
    exit(EXIT_SUCCESS);
}

int main(void)
{
    if(gettimeofday(&t1, NULL) == -1) {
	perror("gettimeofday error");
	exit(EXIT_FAILURE);
    }

    struct sigaction act;

    memset(&act, '\0', sizeof(act));

    act.sa_sigaction = &sig_handler;

    act.sa_flags = SA_SIGINFO;

    if (sigaction(SIGTERM, &act, NULL) == -1) {
	perror("sigaction");
	exit(EXIT_FAILURE);
    }

    int listenfd = 0, connfd = 0;

    struct sockaddr_in serv_addr;
    struct sockaddr_in client_addr;
    socklen_t socksize = sizeof(struct sockaddr_in);
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&serv_addr, '0', sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(50001);

    bind(listenfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));

    if (listen(listenfd, 10) == -1) {
	perror("Failed to listen");
	exit(EXIT_FAILURE);
    }

    puts("Waiting for incoming connections...");
    while (1) {
	printf("Waiting for a client to connect...\n");
	connfd =
	    accept(listenfd, (struct sockaddr *) &client_addr, &socksize);
	printf("Connection accepted...\n");

	pthread_t sniffer_thread;

	if (pthread_create
	    (&sniffer_thread, NULL, client_handler,
	     (void *) &connfd) < 0) {
	    perror("could not create thread");
	    exit(EXIT_FAILURE);
	}
	printf("Handler assigned\n");
    }

    exit(EXIT_SUCCESS);
} 

void *client_handler(void *socket_desc)
{
    int connfd = *(int *) socket_desc;
    char i = '0';
    char input[0];
    char *input_ptr = input;

    send_hello(connfd);

    do {
	input_ptr = get_option(connfd);
	i = input_ptr[0];

	switch (i) {
	case '0':
	    printf("\n");
	    break;
	case '1':
	    send_student_id(connfd);
	    printf("\n");
	    break;
	case '2':
	    get_and_send_server_time(connfd);
	    printf("\n");
	    break;
	case '3':
	    get_and_send_uname(connfd);
	    printf("\n");
	    break;
	case '4':
	    list_files(connfd);
	    printf("\n");
	    break;
	case '5':
	    printf("Goodbye!\n");
	    break;
	default:
	    printf("Invalid choice\n");
	    break;
	}
    } while (i != '5');

    shutdown(connfd, SHUT_RDWR);
    close(connfd);

    printf("Thread %lu exiting\n", (unsigned long) pthread_self());

    shutdown(connfd, SHUT_RDWR);
    close(connfd);

    return 0;
}

void send_hello(int socket)
{
    char hello_string[] = "hello SP student";

    size_t n = strlen(hello_string) + 1;

    if(writen(socket, (unsigned char *) &n, sizeof(size_t)) <= 0) {
	perror("could not write size to client");
	exit(EXIT_FAILURE);
    }	
    if(writen(socket, (unsigned char *) hello_string, n) <= 0) {
	perror("could not write greeting to client");
	exit(EXIT_FAILURE);
    }	  
}

char *get_ip_address() {

    int fd;
    struct ifreq ifr;

    fd = socket(AF_INET, SOCK_DGRAM, 0);

    ifr.ifr_addr.sa_family = AF_INET;

    strncpy(ifr.ifr_name, "eth0", IFNAMSIZ-1);

    ioctl(fd, SIOCGIFADDR, &ifr);

    close(fd);

    char *ip_address = inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr);
    return ip_address;
}

void get_and_send_server_time(int socket) {

    time_t t;
    if ((t = time(NULL)) == -1) {
	perror("time error");
	exit(EXIT_FAILURE);
    }

    struct tm *tm;
    if ((tm = localtime(&t)) == NULL) {
	perror("localtime error");
	exit(EXIT_FAILURE);
    }    

    char *time = asctime(tm);
    char time_to_send[32];
    strcpy(time_to_send, "");
    strcat(time_to_send, time);
   
    size_t n = strlen(time_to_send) + 1;

    printf("The current server time is: %s\n", time_to_send);

    if(writen(socket, (unsigned char *) &n, sizeof(size_t)) <= 0) {
	perror("could not write size to client");
	exit(EXIT_FAILURE);
    }	
    if(writen(socket, (unsigned char *) time_to_send, n) <= 0) {
	perror("could not write server time to client");
	exit(EXIT_FAILURE);
    }
}

void get_and_send_uname(int socket) {

    struct utsname *uts;
    uts = (struct utsname *) malloc(sizeof(struct utsname));

    if (uname(uts) == -1) {
	perror("uname error");
	exit(EXIT_FAILURE);
    }

    size_t payload_length = sizeof(struct utsname);

    if(writen(socket, (unsigned char *) &payload_length, sizeof(size_t)) <= 0) {
	perror("could not write size to client");
	exit(EXIT_FAILURE);
    }	
    if(writen(socket, (unsigned char *) uts, payload_length) <= 0) {
	perror("could not write uname information to client");
	exit(EXIT_FAILURE);
    }

    free(uts);
}

void send_student_id(int socket) {

    char *ip = get_ip_address();
    char *id = "\nS1423789";
    char full_id[64];
    strcpy(full_id, "");
    strcat(full_id, ip);
    strcat(full_id, id);

    size_t n = strlen(full_id) + 1;

    printf("IP address and SID: %s\n", full_id);
    if(writen(socket, (unsigned char *) &n, sizeof(size_t)) <= 0) {
	perror("could not write size to client");
	exit(EXIT_FAILURE);
    }	
    if(writen(socket, (unsigned char *) full_id, n) <= 0) {
	perror("could not write id to client");
	exit(EXIT_FAILURE);
    }
}

char *get_option(int socket) {

    char input[2];
    size_t k;

    if(readn(socket, (unsigned char *) &k, sizeof(size_t)) == 0) {
	perror("could not read size");	
	exit(EXIT_FAILURE);
    }
    if(readn(socket, (unsigned char *) input, k) == 0) {
	perror("could not read option");	
	exit(EXIT_FAILURE);
    }

    printf("Size of received option is: %zu bytes. \n", k);
    char *i = input;
    return i;
} 

void stat_file(char *file) {

    struct stat sb;

    if (stat(file, &sb) == -1) {
	perror("stat");
	exit(EXIT_FAILURE);
    }

    printf("File type:                ");

    switch (sb.st_mode & S_IFMT) {
    case S_IFBLK:
	printf("block device\n");
	break;
    case S_IFCHR:
	printf("character device\n");
	break;
    case S_IFDIR:
	printf("directory\n");
	break;
    case S_IFIFO:
	printf("FIFO/pipe\n");
	break;
    case S_IFLNK:
	printf("symlink\n");
	break;
    case S_IFREG:
	printf("regular file\n");
	break;
    case S_IFSOCK:
	printf("socket\n");
	break;
    default:
	printf("unknown?\n");
	break;
    }

    printf("I-node number:            %ld\n", (long) sb.st_ino);

    printf("Mode:                     %lo (octal)\n",
	   (unsigned long) sb.st_mode);

    printf("Link count:               %ld\n", (long) sb.st_nlink);
    printf("Ownership:                UID=%ld   GID=%ld\n",
	   (long) sb.st_uid, (long) sb.st_gid);

    printf("Preferred I/O block size: %ld bytes\n", (long) sb.st_blksize);
    printf("File size:                %lld bytes\n",
	   (long long) sb.st_size);
    printf("Blocks allocated:         %lld\n", (long long) sb.st_blocks);

    printf("Last status change:       %s", ctime(&sb.st_ctime));
    printf("Last file access:         %s", ctime(&sb.st_atime));
    printf("Last file modification:   %s", ctime(&sb.st_mtime));

    printf("\n");
}

void list_files(int socket) {

    struct dirent **namelist;
    int n;
    char file_name[32];
    char *file_name_ptr;
    char list_of_files[100];
    file_name_ptr = (char *) file_name;
    
    if ((n = scandir(".", &namelist, NULL, alphasort)) == -1)
	perror("scandir");
    else {
	
    	strcpy(list_of_files, "Regular Files: ");
 
	while (n--) {

	    printf("File name:		  %s\n", namelist[n]->d_name);
	    file_name_ptr = namelist[n]->d_name;
	    stat_file(namelist[n]->d_name);
	    strcat(list_of_files, file_name_ptr);
	    strcat(list_of_files, "*");
	    free(namelist[n]);
	}

	size_t k = strlen(list_of_files) + 1;

	if(writen(socket, (unsigned char *) &k, sizeof(size_t)) <= 0) {
	    perror("could not write size to client");
	    exit(EXIT_FAILURE);
        }	
        if(writen(socket, (unsigned char *) list_of_files, k) <= 0) {
	    perror("could not write greeting to client");
	    exit(EXIT_FAILURE);
        }

	free(namelist);		
    }

}
