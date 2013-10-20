/*
** client.c -- a stream socket client demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <arpa/inet.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>

//#define PORT "3490" // the port client will be connecting to 
#define MAXDATASIZE 10000000 // max number of bytes we can get at once 

/* Return 1 if the difference is negative, otherwise 0.  */
int timeval_subtract(struct timeval *result, struct timeval *t2, struct timeval *t1)
{
    long int diff = (t2->tv_usec + 1000000 * t2->tv_sec) - (t1->tv_usec + 1000000 * t1->tv_sec);
    result->tv_sec = diff / 1000000;
    result->tv_usec = diff % 1000000;

    return (diff<0);
}

void timeval_print(struct timeval *tv)
{
    char buffer[30];
    time_t curtime;

    printf("%ld.%06ld", tv->tv_sec, tv->tv_usec);
    curtime = tv->tv_sec;
    strftime(buffer, 30, "%m-%d-%Y  %T", localtime(&curtime));
    printf(" = %s.%06ld\n", buffer, tv->tv_usec);
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{
	int sockfd, numbytes;  
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];

	if (argc != 4) {
	    fprintf(stderr,"usage: client hostname port pathname\n");
	    exit(1);
	}
	char* port = argv[2];
	char* pathname = argv[3];
	
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo(argv[1], port, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and connect to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("client: connect");
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		return 2;
	}

	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
			s, sizeof s);
	printf("client: connecting to %s\n", s);

	freeaddrinfo(servinfo); // all done with this structure

	//char request[MAXDATASIZE];
	//strcpy(request, "GET ");
	//strcat(request, pathname);
	//printf("pathname len: %d\n", strlen(pathname));
	int request_sz = strlen("GET ") + strlen(pathname) + strlen("\r\n") + 1;
	char* request = malloc(request_sz);
	memset(request, 0, request_sz);
	sprintf(request, "GET %s\r\n", pathname);
	//printf("request: %s\n", request);
	//printf("request len: %d\n", strlen(request));
	//printf("size: %d\n", sizeof(request));
	
	/*int lastIndex = 4 + strlen(pathname);
	request[lastIndex] = '\r';	
	request[lastIndex+1] = '\n';
	request[lastIndex+2] = '\0';*/
	//time_t start = time(NULL);
	//clock_t start = clock();
	//sleep(3);
	struct timeval tvBegin, tvEnd, tvDiff;
	// begin
    gettimeofday(&tvBegin, NULL);
    //timeval_print(&tvBegin);
	
	if(send(sockfd, request, strlen(request)+1, 0) == -1){
		perror("send");
	}
    free(request);
    
    /*char buf[MAXDATASIZE];
    memset(buf, 0, MAXDATASIZE);    
	if ((numbytes = recv(sockfd, buf, MAXDATASIZE, 0)) == -1) {
	    perror("recv");
	    exit(1);
	}*/
	
	char buffer[MAXDATASIZE];
    memset(buffer, 0, MAXDATASIZE);
    int current_bytes;
    int sum_bytes = 0;
    int size = MAXDATASIZE;

	while (1) {
        current_bytes = recv(sockfd, buffer + sum_bytes, size - sum_bytes, 0);
        if (current_bytes == -1) {
        	perror("recv");
	        exit(1);
        }
        if (current_bytes == 0) {
        	break;
        }
        sum_bytes += current_bytes;
    }   
	
	//buf[numbytes] = '\0';
	//printf("client: received\n%s", buf);
	printf("%s\n", buffer);

	//end
    gettimeofday(&tvEnd, NULL);
    //timeval_print(&tvEnd);
	
	// diff
    timeval_subtract(&tvDiff, &tvEnd, &tvBegin);
    //printf("%ld.%06ld\n", tvDiff.tv_sec, tvDiff.tv_usec);
	
	//time_t end = time(NULL);
	//clock_t end = clock();
	//float seconds = (float)(end - start) / CLOCKS_PER_SEC;
	//double seconds = ((double) (end - start) * 1000) / CLOCKS_PER_SEC;
	//printf("%.5f seconds\n", seconds);
	
	/*int k = 0;
	for(k; k < MAXDATASIZE; k++){
	    if(buffer[k] == '\0')
	        break;
	}
	printf("k = %d\n", k);*/
	
	close(sockfd);
    
	return 0;
}

