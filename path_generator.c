/*
 * Copyright (c) 1987, 1993, 1994
 *      The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "path_generator.h"
#include "cqueue.h"

#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <math.h>

#define BUFSIZE 1024
#define UDP_LISTEN_PORT 8001
#define MAX_PATHS 100 //Maximum number of pre-calculated paths
#define RADIUS 6378160 //Radius of Earth in meters

pthread_mutex_t path_mutex; //Mutex to control path messages
pthread_mutex_t motion_mutex; //Mutex to control motion path list (list of coordinates)

t_motions_list *motions_list;

/*
 * error - wrapper for perror
 */
void error(char *msg) {
	perror(msg);
	exit(1);
}

double deg2rad(double deg) {
  return deg * (M_PI/180);
}

/*
 * Find distance in meters between two coordinates
 */
double get_distance(float lat0, float lon0, float lat1, float lon1) {
  double dLat = deg2rad(lat1 - lat0);
  double dLon = deg2rad(lon1 - lon0);
  double a =
    sin(dLat / 2) * sin(dLat / 2) +
    cos(deg2rad(lat0)) * cos(deg2rad(lat1)) *
    sin(dLon / 2) * sin(dLon / 2);
  double c = 2 * atan2(sqrt(a), sqrt(1 - a));
  double d = RADIUS * c; // distance in m
  return d;
}


/*
 * Searches inside string input for substring str.
 * Both input and str should be zero terminated.
 * If found then return index of the input that matches
 * beginning of the str, otherwise reurns -1
 */
int findsbstr(const char *input, const char *str)
{
    int pos = 0;
    int i = 0;
    int len_input = strlen(input);
    int len_str = strlen(str);

    if(len_input <= 0 || len_str <= 0) return -1;

    while (pos + len_str <= len_input) {

        for (i=0; i<len_str; i++)
            if (str[i] != input[pos+i]) goto next;
        return pos;
next:
        pos++;
    }

    return -2;
}

/*
 * Return next motion path or null if there is nothing
 */
t_motion* get_next_motion_path(){
	pthread_mutex_lock(&motion_mutex);

	if(!motions_list) {
		return NULL;
	} else {
		if (motions_list->motion) {
			return motions_list->motion;
		}
	}


	pthread_mutex_unlock(&motion_mutex);
}

/*
 * This is a main path reader function.
 * It should read path from the queue and if it is not empty
 * convert it to set of coordinates
 */
void* path_reader(void *arg)
{
	char buf[BUFSIZE];
	int res = 0;

	while(1)
	{
		bzero(&buf, BUFSIZE);

		pthread_mutex_lock(&path_mutex);
		res = QueueGet(buf, BUFSIZE);
		pthread_mutex_unlock(&path_mutex);

		if (res == 0) {
			/* 0 means that we got some data, so we have to process it*/
			printf("path reader got path request: %s\n", buf);

			/* Parse PATH message and extract tokens */
			char *p = strtok(buf, ";");
			int i = 0;
			char *array[7];

			while(p != NULL){
				array[i++] = p;
				p = strtok(NULL, ";");
				printf("array[%d]=%s\n", i, array[i - 1]);
			};

			if (i != 7) {
				printf("Error! Can't parse PATH message: %s;\n", buf);
				continue;
			}


			/* Convert extracted tokens to float values */
			float lat0;
			sscanf(array[1], "%1f", &lat0);

			float lon0;
			sscanf(array[2], "%1f", &lon0);

			float lat1;
			sscanf(array[3], "%1f", &lat1);

			float lon1;
			sscanf(array[4], "%1f", &lon1);

			float speed;
			sscanf(array[5], "%1f", &speed);

			float pause;
			sscanf(array[6], "%1f", &pause);

			double d = get_distance(lat0, lon0, lat1, lon1);
			printf("distance in meters d: %lf\n", d);


		} else {
			//sleep, queue is empty
			usleep(100);
		}
	}

	printf("path reader Finished OK\n");

	pthread_exit(NULL);
}

/*
 * Process message received by the UDP server.
 * It should either start calculation of the path
 * or send current location
 */
int process_message(char* buf, char* msg_back){
	int pos = 0;
	int res = 0;

	printf("process_message buf: %s\n", buf);

	/* If that is new path message - add it to the queue and return OK*/
	pos = findsbstr(buf, "PATH");
	printf("process_message pos: %d\n", pos);
	if (pos == 0) {
		pthread_mutex_lock(&path_mutex);
		res = QueuePut(buf);
		pthread_mutex_unlock(&path_mutex);

		if (res < 0) {
			printf("Error! Queue is full");
			sprintf(msg_back, "%s", "Error! Queue is full");
		}

		sprintf(msg_back, "%s", "ADDED OK");
		return 1;
	}

	/* If that is new status message - return current location*/
	pos = findsbstr(buf, "CUR_LOC");
	if (pos == 0) {
		sprintf(msg_back, "%s", "??? GET CURRENT LOCATION");
		return 1;
	}

	sprintf(msg_back, "%s", "ERROR! Got unknown message");

	return 1;
}

/*
 * handle message received by the thread.
 * It should ALWAYS sent response message
 */
void* start_udp_server(void *arg) {
	int sockfd; /* socket */
	int portno = UDP_LISTEN_PORT; /* port to listen on */
	int clientlen; /* byte size of client's address */
	struct sockaddr_in serveraddr; /* server's addr */
	struct sockaddr_in clientaddr; /* client addr */
	struct hostent *hostp; /* client host info */
	char buf[BUFSIZE]; /* message in buf */
	char msg_back[BUFSIZE]; /* message out*/
	char *hostaddrp; /* dotted decimal host addr string */
	int optval; /* flag value for setsockopt */
	int n; /* message byte size */
	int res; /* flag indicating if there is a result message */


	/*Initialize path reader thread */
	pthread_t tid;
	res = pthread_create(&tid, NULL, &path_reader, NULL);
	if (res != 0)
		printf("Can't create path_reader thread :[%s]", strerror(res));

	/*
	 * socket: create the parent socket
	 */
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0)
		error("ERROR opening socket");

	/* setsockopt: Handy debugging trick that lets
	 * us rerun the server immediately after we kill it;
	 * otherwise we have to wait about 20 secs.
	 * Eliminates "ERROR on binding: Address already in use" error.
	 */
	optval = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const void *) &optval,
			sizeof(int));

	/*
	 * build the server's Internet address
	 */
	bzero((char *) &serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons((unsigned short) portno);

	hostaddrp = inet_ntoa(clientaddr.sin_addr);
	if (hostaddrp == NULL) error("ERROR on inet_ntoa\n");
	printf("UDP Server started %s:(%d)\n", hostaddrp, portno);

	/*
	 * bind: associate the parent socket with a port
	 */
	if (bind(sockfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0)
		error("ERROR on binding");

	/*
	 * main loop: wait for a datagram, then echo it
	 */
	clientlen = sizeof(clientaddr);
	while (1) {

		/*
		 * recvfrom: receive a UDP datagram from a client
		 */
		bzero(buf, BUFSIZE);
		bzero(msg_back, BUFSIZE);

		n = recvfrom(sockfd, buf, BUFSIZE, 0, (struct sockaddr *) &clientaddr,
				&clientlen);

		if (n < 0)
			error("ERROR in recvfrom");

		/*
		 * gethostbyaddr: determine who sent the datagram
		 */
		hostp = gethostbyaddr((const char *) &clientaddr.sin_addr.s_addr,
				sizeof(clientaddr.sin_addr.s_addr), AF_INET);

		if (hostp == NULL)
			error("ERROR on gethostbyaddr");

		hostaddrp = inet_ntoa(clientaddr.sin_addr);
		if (hostaddrp == NULL)
			error("ERROR on inet_ntoa\n");

		printf("server received %Zu/%d bytes, from %s, data: %s\n", strlen(buf), n, hostaddrp, buf);

		if (n > 0) {
			res = process_message(buf, msg_back);
		}

		/* send result back to the client */
		if (res > 0) {
			n = sendto(sockfd, msg_back, strlen(msg_back), 0, (
					struct sockaddr *) &clientaddr, clientlen);
			if (n < 0) error("ERROR in sendto");
		}

	}

	pthread_exit(NULL);
}

