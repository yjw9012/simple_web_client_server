/*
** server.c -- a stream socket server demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/stat.h>
#include <malloc.h>

//#define PORT "3490"  // the port users will be connecting to

#define BACKLOG 10	 // how many pending connections queue will hold
#define MAXREQUESTSIZE 100
#define MAXCONTENTLEN 10

const char *HTTP_200_STRING = "HTTP 200 OK\r\nContent-Length: ";
const char *END_CHARACTERS = "\r\n\r\n";
const char *HTTP_404_STRING = "HTTP 404 Not Found\r\n\r\n";
const char *HTTP_400_STRING = "HTTP 400 Bad Request\r\n\r\n";

/*void handler(int sig){
	pid_t pid;
	int stat;
	while((pid = waitpid(-1,&stat,WNOHANG)) > 0);

	return;
}*/

void sigchld_handler(int s)
{
	while(waitpid(-1, NULL, WNOHANG) > 0);
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

char* check_form(char* request){
    if(strncmp("GET ", request, 4) != 0)
        return NULL; 
    
    char* CR = strstr(request, "\r\n");
    if(CR == NULL){
        //printf("CR is null.\n"); 
        return NULL;
    }

    // pathname is empty
    if(request + 4 >= CR){
        //printf("pathname is empty.\n"); 
        return NULL;
    }
    
    int index = CR - request + 2;
    
    //printf("index: %d\n", CR-request);
    for(; index < MAXREQUESTSIZE; index++){
        if(request[index] != '\0'){
            //printf("%d NULL\n", index); 
            return NULL;
        }
    } 
    
    return CR;
}

int main(int argc, char *argv[])
{
	int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	struct sigaction sa;
	int yes=1;
	char s[INET6_ADDRSTRLEN];
	int rv;

	if (argc != 2) {
	    fprintf(stderr,"usage: server port\n");
	    exit(1);
	}
	
	char* port = argv[1];
	int numbytes;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, port, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
				sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind");
			continue;
		}

		break;
	}

	if (p == NULL)  {
		fprintf(stderr, "server: failed to bind\n");
		return 2;
	}

	freeaddrinfo(servinfo); // all done with this structure

	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}

	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}
	
	//signal(SIGCHLD, handler);
	//signal(SIGCHLD, SIG_IGN);
	printf("server: waiting for connections...\n");

	while(1) {  // main accept() loop
		sin_size = sizeof their_addr;
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		if (new_fd == -1) {
			perror("accept");
			continue;
		}

		inet_ntop(their_addr.ss_family,
			get_in_addr((struct sockaddr *)&their_addr),
			s, sizeof s);
		printf("server: got connection from %s\n", s);

		if (!fork()) { // this is the child process
		    close(sockfd); // child doesn't need the listener
		    
		    char* request = malloc(MAXREQUESTSIZE);
	        memset(request, 0, MAXREQUESTSIZE);
			
			if((numbytes = recv(new_fd, request, MAXREQUESTSIZE, 0)) == -1){
				perror("recv");
				exit(1);	
			}
			//request[numbytes] = '\0';
			//printf("server: received '%s'\n", buf);
			printf("%s", request);
	        char* CR = check_form(request);
			if(CR == NULL){
                if (send(new_fd, HTTP_400_STRING, strlen(HTTP_400_STRING+1), 0) == -1){
				    perror("send");
				}
				free(request);
			    close(new_fd);
			    exit(0);
			}
			
			char pathname[MAXREQUESTSIZE];
			memset(pathname, 0, MAXREQUESTSIZE);
			strncpy(pathname, request+4, CR-request-4);
			//char* temp = buf+4*sizeof(char);
			//printf("temp: %s\n", temp);	
			//printf("%d", i);
			//printf("pathname = '%s'\n", pathname);			
            //printf("pathname: %s\n",pathname);
           
            char* response = NULL;
            int total_size;
            struct stat s;
            
			if(stat(pathname,&s) == 0){
			    if(s.st_mode & S_IFREG){
			        FILE* file = fopen(pathname, "r");
				    //printf("successful reading file\n");
				    fseek(file, 0, SEEK_END);
				    int filesize = ftell(file);
				    fseek(file, 0, SEEK_SET);
				
				    //printf("pos = %d\n", pos);
				    char* data = malloc(filesize);
			        memset(data, 0, filesize);
				
				    int j = 0;
				    while(!feof(file)){
					    //char*s = malloc(1024);
					    //fgets(s,1024,file);
					    //printf("len = %d\n", strlen(s));
					    //s[strlen(s)] = '\0';
					    //strcat(data,s);
					    //printf("string: %s", s);
					    //free(s);
					    data[j] = fgetc(file);
					    j++;					
				    }
				    //data[j-1] = '\0';
				    data[j-2] = '\0';
				    fclose(file);
				
				    total_size = strlen(HTTP_200_STRING) + MAXCONTENTLEN + strlen(END_CHARACTERS) + filesize + 1;
				    response = malloc(total_size);
				    memset(response, 0, total_size);
				    sprintf(response, "%s%d%s%s", HTTP_200_STRING, j-2, END_CHARACTERS, data);
				    free(data);
				    //strcat(response, pos);
				    //strcat(response, "\r\n\r\n");
				    //strcat(response, data);
			    }
			}	
			//printf("data is %s", data);			
			//printf("data len = %d\n", strlen(data));
			//printf("response\n%s",response);
			if(response != NULL){
			    if (send(new_fd, response, total_size, 0) == -1){
				    perror("send");
                }
                //printf("response len: %d\n", strlen(response));
				free(response);
		    }
		    else {
		        if (send(new_fd, HTTP_404_STRING, strlen(HTTP_404_STRING)+1, 0) == -1)
				    perror("send");
		    }
		    free(request);
			close(new_fd);
			exit(0);
		}
		close(new_fd);  // parent doesn't need this
	}

	return 0;
}

