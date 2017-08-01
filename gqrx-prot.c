#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pwd.h>
#include <stdbool.h>
#include <linux/limits.h>
#include <math.h>
#include "gqrx-prot.h"

// 
// error - wrapper for perror
//
void error(char *msg) {
    perror(msg);
    exit(0);
}

//
// Connect
//
int Connect (char *hostname, int portno)
{
    int sockfd, n;
    struct sockaddr_in serveraddr;
    struct hostent *server;
  
    
    /* socket: create the socket */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");

    /* gethostbyname: get the server's DNS entry */
    server = gethostbyname(hostname);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host as %s\n", hostname);
        exit(0);
    }

    /* build the server's Internet address */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
	  (char *)&serveraddr.sin_addr.s_addr, server->h_length);
    serveraddr.sin_port = htons(portno);

    /* connect: create a connection with the server */
    if (connect(sockfd, (const struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0) 
      error("ERROR connecting");

    return sockfd;
}
//
// Send
//
bool Send(int sockfd, char *buf)
{
    int n;

    n = write(sockfd, buf, strlen(buf));
    if (n < 0) 
      error("ERROR writing to socket");
    return true;
}

//
// Recv
//
bool Recv(int sockfd, char *buf)
{
    int n;

    n = read(sockfd, buf, BUFSIZE);
    if (n < 0) 
      error("ERROR reading from socket");
    buf[n]= '\0';
    return true;
}


//
// GQRX Protocol
//
bool GetCurrentFreq(int sockfd, long *freq)
{
    char buf[BUFSIZE];
    
    Send(sockfd, "f\n");
    Recv(sockfd, buf);

    if (strcmp(buf, "RPRT 1") == 0 )
        return false;
    
    sscanf(buf, "%ld", freq);
    return true;
}
bool SetFreq(int sockfd, long freq)
{
    char buf[BUFSIZE];
    
    sprintf (buf, "F %ld\n", freq);
    Send(sockfd, buf);
    Recv(sockfd, buf);

    if (strcmp(buf, "RPRT 1") == 0 )
        return false;
    
    long freq_current = 0;
    do
    {
        GetCurrentFreq(sockfd, &freq_current);
    } while (freq_current != freq);

    return true;
}

bool GetSignalLevel(int sockfd, double *dBFS)
{
    char buf[BUFSIZE];
    
    Send(sockfd, "l\n");
    Recv(sockfd, buf);

    if (strcmp(buf, "RPRT 1") == 0 )
        return false;
    
    sscanf(buf, "%lf", dBFS);
    *dBFS = round((*dBFS) * 10)/10;
    
    if (*dBFS == 0.0)
        return false;
    return true;
}

bool GetSquelchLevel(int sockfd, double *dBFS)
{
    char buf[BUFSIZE];
    
    Send(sockfd, "l SQL\n");
    Recv(sockfd, buf);

    if (strcmp(buf, "RPRT 1") == 0 )
        return false;
    
    sscanf(buf, "%lf", dBFS);
    *dBFS = round((*dBFS) * 10)/10;

    return true;
}
//
// GetSignalLevelEx
// Get a bunch of sample with some delay and calculate the mean value
//
bool GetSignalLevelEx(int sockfd, double *dBFS, int n_samp)
{
    double temp_level;
    *dBFS = 0;
    int errors = 0;
    for (int i = 0; i < n_samp; i++)
    {
        if ( GetSignalLevel(sockfd, &temp_level) )
            *dBFS = *dBFS + temp_level; 
        else
            errors++;
        usleep(1000);
    }
    *dBFS = *dBFS / (n_samp - errors);
    return true;
}