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

// thread function
void *client_handler(void *);

//send student id function
void send_student_id(int);

//send greeting function
void send_hello(int);

//get ip address function
char *get_ip_address();

//send time on server function
void get_and_send_server_time(int);

//read option function
char *get_option(int);

//send uname information function
void get_and_send_uname(int);

//generate file information function
void stat_file(char *);

//send files function
void list_files(int);


//creates 2 global timeval structures
struct timeval t1, t2;


//handles a SIGTERM signal when received
static void sig_handler(int sig, siginfo_t *siginfo, void *context)
{
    //generates end time for server execution
    if(gettimeofday(&t2, NULL) == -1) { 
	perror("gettimeofday error");
	exit(EXIT_FAILURE);
    }

    //shows total server execution time in seconds and microseconds.
    printf("Total execution time = %f seconds\n",(double) (t2.tv_usec - t1.tv_usec) / 1000000 + (double) (t2.tv_sec - t1.tv_sec));

    //ends server process
    printf("Server shutting down...\n");
    exit(EXIT_SUCCESS);
}

// you shouldn't need to change main() in the server except the port number
int main(void)
{

    //generates start time for server execution
    if(gettimeofday(&t1, NULL) == -1) {
	perror("gettimeofday error");
	exit(EXIT_FAILURE);
    }

    struct sigaction act;

    memset(&act, '\0', sizeof(act));

    //points to the sig_handler() function
    act.sa_sigaction = &sig_handler;

    // the SA_SIGINFO flag tells sigaction() to use the sa_sigaction field, not sa_handler
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
    // end socket setup

    //Accept and incoming connection
    puts("Waiting for incoming connections...");
    while (1) {
	printf("Waiting for a client to connect...\n");
	connfd =
	    accept(listenfd, (struct sockaddr *) &client_addr, &socksize);
	printf("Connection accepted...\n");

	pthread_t sniffer_thread;
        // third parameter is a pointer to the thread function, fourth is its actual parameter
	if (pthread_create
	    (&sniffer_thread, NULL, client_handler,
	     (void *) &connfd) < 0) {
	    perror("could not create thread");
	    exit(EXIT_FAILURE);
	}
	//Now join the thread , so that we dont terminate before the thread
	//pthread_join( sniffer_thread , NULL);
	printf("Handler assigned\n");
    }

    // never reached...
    // ** should include a signal handler to clean up
    exit(EXIT_SUCCESS);
} // end main()


// thread function - one instance of each for each connected client
void *client_handler(void *socket_desc)
{
    //Get the socket descriptor
    int connfd = *(int *) socket_desc;
    char i = '0';
    char input[0];
    char *input_ptr = input;

    //sends greeting to client
    send_hello(connfd);

    //do-while loop with switch case statement: takes option from client and executes corresponding function for the client's thread. Exits and closes the thread when client selects option 5.
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
    } while (i != '5'); //end do-while loop

    shutdown(connfd, SHUT_RDWR);
    close(connfd);

    printf("Thread %lu exiting\n", (unsigned long) pthread_self());

    // always clean up sockets gracefully
    shutdown(connfd, SHUT_RDWR);
    close(connfd);

    return 0;
}  // end client_handler()


// how to send a string
void send_hello(int socket)
{
    //creates string
    char hello_string[] = "hello SP student";

    //gets string size
    size_t n = strlen(hello_string) + 1;

    //writes size of string, and string itself to the client socket
    if(writen(socket, (unsigned char *) &n, sizeof(size_t)) <= 0) {
	perror("could not write size to client");
	exit(EXIT_FAILURE);
    }	
    if(writen(socket, (unsigned char *) hello_string, n) <= 0) {
	perror("could not write greeting to client");
	exit(EXIT_FAILURE);
    }	  
} // end send_hello()


// get ip address of server and output as string pointer
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
} //end get_ip_address()


//gets server time and sends to client as a string
void get_and_send_server_time(int socket) {

    time_t t;    // always look up the manual to see the error conditions
    //  here "man 2 time"
    if ((t = time(NULL)) == -1) {
	perror("time error");
	exit(EXIT_FAILURE);
    }

    // localtime() is in standard library so error conditions are
    //  here "man 3 localtime"
    struct tm *tm;
    if ((tm = localtime(&t)) == NULL) {
	perror("localtime error");
	exit(EXIT_FAILURE);
    }    

    //assigns the current time to a string pointer, which is then concatenated into a sendable string
    char *time = asctime(tm);
    char time_to_send[32];
    strcpy(time_to_send, "");
    strcat(time_to_send, time);
    
    //gets size of time string to be sent to the client
    size_t n = strlen(time_to_send) + 1;

    printf("The current server time is: %s\n", time_to_send);

    //writes the string size, and the time string to the client socket
    if(writen(socket, (unsigned char *) &n, sizeof(size_t)) <= 0) {
	perror("could not write size to client");
	exit(EXIT_FAILURE);
    }	
    if(writen(socket, (unsigned char *) time_to_send, n) <= 0) {
	perror("could not write server time to client");
	exit(EXIT_FAILURE);
    }
} //end get_and_send_server_time()


// gets uname information and sends it to the client as a string
void get_and_send_uname(int socket) {

    //allocates memory for the utsname structure
    struct utsname *uts;
    uts = (struct utsname *) malloc(sizeof(struct utsname));

    //gets uname information and checks for errors
    if (uname(uts) == -1) {
	perror("uname error");
	exit(EXIT_FAILURE);
    }

    //assigns size of utsname
    size_t payload_length = sizeof(struct utsname);

    //writes size of uname information, and uname information to client socket
    if(writen(socket, (unsigned char *) &payload_length, sizeof(size_t)) <= 0) {
	perror("could not write size to client");
	exit(EXIT_FAILURE);
    }	
    if(writen(socket, (unsigned char *) uts, payload_length) <= 0) {
	perror("could not write uname information to client");
	exit(EXIT_FAILURE);
    }

    //frees up allocated memory
    free(uts);
} //end get_and_send_uname()


//sends ip address and student id to client as a string
void send_student_id(int socket) {

    //concatenates ip address and student id together
    char *ip = get_ip_address();
    char *id = "\nS1423789";
    char full_id[64];
    strcpy(full_id, "");
    strcat(full_id, ip);
    strcat(full_id, id);
    
    //gets size of concatenated string
    size_t n = strlen(full_id) + 1;

    //sends both size, and concatenated string to client
    printf("IP address and SID: %s\n", full_id);
    if(writen(socket, (unsigned char *) &n, sizeof(size_t)) <= 0) {
	perror("could not write size to client");
	exit(EXIT_FAILURE);
    }	
    if(writen(socket, (unsigned char *) full_id, n) <= 0) {
	perror("could not write id to client");
	exit(EXIT_FAILURE);
    }
} //end send_student_id()


//gets input option from client and returns it as a string pointer
char *get_option(int socket) {

    char input[2];
    size_t k;

    //reads input option and size of option from client
    if(readn(socket, (unsigned char *) &k, sizeof(size_t)) == 0) {
	perror("could not read size");	
	exit(EXIT_FAILURE);
    }
    if(readn(socket, (unsigned char *) input, k) == 0) {
	perror("could not read option");	
	exit(EXIT_FAILURE);
    }
    
    //reports option size and returns option as string pointer
    printf("Size of received option is: %zu bytes. \n", k);
    char *i = input;
    return i;
} //end get_option()


//gets file information
void stat_file(char *file) {

    struct stat sb;

    //gets file status
    if (stat(file, &sb) == -1) {
	perror("stat");
	exit(EXIT_FAILURE);
    }

    printf("File type:                ");

    //switch case statement checks what type the file/directory is and outputs corresponding information
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
    } //end switch case statement

    //prints relevant file information
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
} //end stat_file()


//creates a list of all files in server upload directory, then sends this list as a concatenated 
void list_files(int socket) {

    //goes through upload directory and obtains a namelist of all files/directories
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
        
        //takes each name from the namelist and inserts them into a concatenated string with an * as a separator
	while (n--) {

	    printf("File name:		  %s\n", namelist[n]->d_name);
	    file_name_ptr = namelist[n]->d_name;
	    stat_file(namelist[n]->d_name);
	    strcat(list_of_files, file_name_ptr);
	    strcat(list_of_files, "*");
	    free(namelist[n]);	//NB
	}
        
        //gets size of concatenated string
	size_t k = strlen(list_of_files) + 1;

        //sends the size and the string to the client
	if(writen(socket, (unsigned char *) &k, sizeof(size_t)) <= 0) {
	    perror("could not write size to client");
	    exit(EXIT_FAILURE);
        }	
        if(writen(socket, (unsigned char *) list_of_files, k) <= 0) {
	    perror("could not write greeting to client");
	    exit(EXIT_FAILURE);
        }

	free(namelist);		//NB
    }

} //end list_files()
