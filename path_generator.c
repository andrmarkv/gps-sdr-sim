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
#define RADIUS 6378137 //Radius of Earth in meters

#define ALTITUDE 1.0//Altitude for the generated coordinates

pthread_mutex_t path_mutex; //Mutex to control path messages
pthread_mutex_t motion_mutex; //Mutex to control motion path list (list of coordinates)
pthread_mutex_t stop_flag_mutex; //Mutex to control start/stop flag

t_motions_list *motions_list;
t_motion cur_loc = { 0 };
int stop_flag = 0;

/*
 * error - wrapper for perror
 */
void error(char *msg) {
	perror(msg);
	exit(1);
}

double deg2rad(double deg) {
	return deg * (M_PI / 180);
}

/*
 * Find distance in meters between two coordinates
 */
double get_distance(double lat0, double lon0, double lat1, double lon1) {
	printf("lat0: %lf lon0: %lf lat1: %lf lon1: %lf \n", lat0, lon0, lat1,
			lon1);

	double dLat = deg2rad(lat1 - lat0);
	double dLon = deg2rad(lon1 - lon0);
	double a = sin(dLat / 2) * sin(dLat / 2)
			+ cos(deg2rad(lat0)) * cos(deg2rad(lat1)) * sin(dLon / 2)
					* sin(dLon / 2);

	double c = 2 * atan2(sqrt(a), sqrt(1 - a));
	double d = RADIUS * c; // distance in m
	printf("distance (m): %lf\n", d);
	return d;
}

/*
 * Searches inside string input for substring str.
 * Both input and str should be zero terminated.
 * If found then return index of the input that matches
 * beginning of the str, otherwise reurns -1
 */
int findsbstr(const char *input, const char *str) {
	int pos = 0;
	int i = 0;
	int len_input = strlen(input);
	int len_str = strlen(str);

	if (len_input <= 0 || len_str <= 0)
		return -1;

	while (pos + len_str <= len_input) {

		for (i = 0; i < len_str; i++)
			if (str[i] != input[pos + i])
				goto next;
		return pos;
		next: pos++;
	}

	return -2;
}

/*
 * Return next motion path or null if there is nothing
 */
t_motion* get_next_motion_path() {
	pthread_mutex_lock(&motion_mutex);

	if (!motions_list) {
		pthread_mutex_unlock(&motion_mutex);
		return NULL;
	} else {
		if (motions_list->motion) {
			pthread_mutex_unlock(&motion_mutex);
			return motions_list->motion;
		}
	}

	pthread_mutex_unlock(&motion_mutex);
	return NULL;
}

/*
 * Set up flag to signal start/stop gps simulation
 * 1 stop flag is set
 * 0 no stop flag
 */
void set_stop_flag(int flag) {
	pthread_mutex_lock(&stop_flag_mutex);

	stop_flag = flag;

	pthread_mutex_unlock(&stop_flag_mutex);
	return;
}

/*
 * Retrieve flag to signal start/stop gps simulation
 * 1 stop flag is set
 * 0 no stop flag
 */
int get_stop_flag() {
	pthread_mutex_lock(&stop_flag_mutex);

	int flag = stop_flag;

	pthread_mutex_unlock(&stop_flag_mutex);
	return flag;
}

/*
 * Add new motion path to the list of motions
 */
int add_motion_path(t_motion* motion) {
	int i = 0;
	pthread_mutex_lock(&motion_mutex);

	/* If list not initialized - initialize and add */
	if (!motions_list) {
		motions_list = malloc(sizeof(t_motions_list));
		memset(motions_list, 0, sizeof(t_motions_list));

		motions_list->motion = motion;
		pthread_mutex_unlock(&motion_mutex);
		return -1;
	}

	/* Find last element in the list */
	t_motions_list *current = motions_list;
	while (1) {
//		printf("TEST 999, i=%d, current=%p, next=%p\n", i, current, current->next);
		sleep(1);
		i++;
		if (current->next == NULL)
			break;
		current = current->next;

	}

	/* create new motion_list, populate it and add it as next element */
	t_motions_list *next = malloc(sizeof(t_motions_list));
	memset(next, 0, sizeof(t_motions_list));

	next->motion = motion;
	current->next = next;

	pthread_mutex_unlock(&motion_mutex);

	return 1;
}

/*
 * Delete specified motion path from the list of motions
 */
int del_motion_path(t_motion* motion) {
	int i = 0;
	pthread_mutex_lock(&motion_mutex);

	if (!motions_list) {
		printf("WARNING! Can't delete motion - list is empty\n");
		pthread_mutex_unlock(&motion_mutex);
		return -1;
	}

	/* Find necessary element in the list */
	t_motions_list *current = motions_list;
	t_motions_list *previous = NULL;
	while (1) {
		i++;
		/* If we found necessary motion */
		if (current->motion == motion) {
			/* if there is no previous - just delete current */
			if (previous == NULL) {
				free(current);
				motions_list = NULL;
				pthread_mutex_unlock(&motion_mutex);
				return 1;
			} else {
				/* If there is no next then delete current */
				if (current->next == NULL) {
					previous->next = NULL;
					free(current);
					pthread_mutex_unlock(&motion_mutex);
					return 1;
				} else {
					/* If there is next then assign it to previous and then delete */
					previous->next = current->next;
					free(current);
					pthread_mutex_unlock(&motion_mutex);
					return 1;
				}
			}
		}

		if (current->next == NULL)
			break;

		previous = current;
	}

	pthread_mutex_unlock(&motion_mutex);

	return 1;
}

/*
 * Print all values of the t_motion structure
 * Very useful for verification
 */
void print_motion(t_motion *motion) {
	int i = 1;
	while (1) {
		if (motion == NULL)
			break;
		if (motion->llh != NULL) {
			printf("step=%d, lat=%lf lon=%lf altitude=%lf\n", i++,
					motion->llh[0], motion->llh[1], motion->llh[2]);
			motion = motion->next;
		}
	}
}

/*
 * Populate motion structure based on the initial point, end point and speed.
 * Returns fully populated list of the coordinates
 */
t_motion* calc_motion(double lat0, double lon0, double lat1, double lon1,
		double speed) {

	/*
	 * find distance between two points on a map
	 */
	double d = get_distance(lat0, lon0, lat1, lon1);

	/*Find out how many steps have to be in the simulation.
	 * Number of steps is equal to the time in secounds * 10
	 * as we increment our movement in 0.1 second steps.
	 * Speed has to be in m/s that is why we have division by 3.6.
	 * Also adding 1 second for rounding as it is better to be a bit
	 * slower then faster
	 */
	int steps = (round(d / (speed / 3.6))) * 10;
	if (steps <= 0)
		steps = 1;

	/*
	 * Find out value of the steps for latitude and longitude
	 */
	double dlat = (lat1 - lat0) / steps;
	double dlon = (lon1 - lon0) / steps;

	/*
	 * Calculate path and populate t_motion structure with points
	 */
	double lat2 = 0;
	double lon2 = 0;
	t_motion *start = malloc(sizeof(t_motion));
	memset(start, 0, sizeof(t_motion));

	if (start != NULL) {
		start->llh[0] = lat0;
		start->llh[1] = lon0;
		start->llh[2] = ALTITUDE;

		t_motion *current = start;

		for (int i = 0; i <= steps; i++) {
			lat2 = lat0 + i * dlat;
			lon2 = lon0 + i * dlon;

			//printf("lat2=%lf, lon2=%lf\n", lat2, lon2);

			t_motion *next = malloc(sizeof(t_motion));
			memset(next, 0, sizeof(t_motion));

			next->llh[0] = lat2;
			next->llh[1] = lon2;
			next->llh[2] = ALTITUDE;

			current->next = next;
			current = next;
		}
	}

	//print_motion(start);
	//printf("lat1=%lf, lon1=%lf\n", lat1, lon1);

	printf("steps=%d, time=%d (sec), dlat=%lf, dlon=%lf\n", steps, steps / 10,
			dlat, dlon);

	return start;
}

/*
 * This is a main path reader function.
 * It should read path from the queue and if it is not empty
 * convert it to set of coordinates
 */
void* path_reader(void *arg) {
	char buf[BUFSIZE];
	int res = 0;

	while (1) {
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

			while (p != NULL) {
				array[i++] = p;
				p = strtok(NULL, ";");
				printf("array[%d]=%s\n", i, array[i - 1]);
			};

			if (i != 7) {
				printf("Error! Can't parse PATH message: %s;\n", buf);
				continue;
			}

			/* Convert extracted tokens to float values */
			double lat0 = atof(array[1]);
			double lon0 = atof(array[2]);
			double lat1 = atof(array[3]);
			double lon1 = atof(array[4]);
			double speed = atof(array[5]);
			//double pause = atof(array[6]);

			/* Get motion path out of coordinates and speed */
			printf("TEST. Calculating new motion path...\n");
			t_motion *motion = calc_motion(lat0, lon0, lat1, lon1, speed);
			//print_motion(motion);

			/* Add new motion path to the list of ready paths */
			printf("TEST. Adding new motion path...\n");
			add_motion_path(motion);
			printf("TEST. Adding new motion path. Done!\n");

		} else {
			//sleep, queue is empty
			usleep(100);
		}
	}

	printf("path reader Finished OK\n");

	pthread_exit(NULL);
}

/*
 * This function should delete all precalculated motions
 * except the current one
 */
void clear_pending_movements() {
	pthread_mutex_lock(&motion_mutex);

	printf("clear_pending_movements, TEST 1\n");

	if (!motions_list) {
		printf("clear_pending_movements, TEST 2\n");
		pthread_mutex_unlock(&motion_mutex);
		return;
	} else {
		printf("clear_pending_movements, TEST 3\n");
		/* skip the first motion_list*/
		t_motions_list *current = motions_list->next;

		printf("clear_pending_movements, TEST 4\n");

		/* if there is no next, just return*/
		if (!current) {
			printf("clear_pending_movements, TEST 5\n");
			pthread_mutex_unlock(&motion_mutex);
			return;
		}

		printf("clear_pending_movements, TEST 6\n");
		t_motions_list *to_be_deleted = NULL;
		while (1) {
			printf("clear_pending_movements, TEST 7\n");
			/* If there is a next motion, delete current and exit*/
			if (!current->next) {
				printf("clear_pending_movements, TEST 8\n");
				free(current);
				break;
			} else {
				/* mark current as deleted and make current = next*/
				to_be_deleted = current;
				current = to_be_deleted->next;
				free(to_be_deleted);
				printf("clear_pending_movements, TEST 9\n");
			}
		}
	}

	printf("clear_pending_movements, TEST 10\n");
	pthread_mutex_unlock(&motion_mutex);
	return;
}

/*
 * Process message received by the UDP server.
 * It should either add path to the queue of the paths that has to be
 * used by path_reader
 * or send current location
 */
int process_message(char* buf, char* msg_back) {
	int pos = 0;
	int res = 0;

	/* If that is new path message - add it to the queue and return OK*/
	pos = findsbstr(buf, "PATH");
	if (pos == 0) {
		printf("got new PATH message: %s\n", buf);
		pthread_mutex_lock(&path_mutex);
		res = QueuePut(buf);
		pthread_mutex_unlock(&path_mutex);

		if (res < 0) {
			printf("Error! Queue is full");
			sprintf(msg_back, "%s", "Error! Queue is full");
		}

		sprintf(msg_back, "%s", "ADDED OK");

		/* clear stop flag */
		set_stop_flag(0);

		return 1;
	}

	/* If that is get location message - return current location*/
	pos = findsbstr(buf, "CUR_LOC");
	if (pos == 0) {
		sprintf(msg_back, "%s;%lf;%lf;%lf", "LOCATION", cur_loc.llh[0],
				cur_loc.llh[1], cur_loc.llh[2]);
		return 1;
	}

	/* If that is stop message - remove all calculated paths*/
	pos = findsbstr(buf, "STOP");
	if (pos == 0) {
		printf("got STOP message\n");
		clear_pending_movements();
		set_stop_flag(1);
		sprintf(msg_back, "%s", "STOP OK");
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
	socklen_t clientlen; /* byte size of client's address */
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
	if (hostaddrp == NULL)
		error("ERROR on inet_ntoa\n");
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

//		printf("server received %Zu/%d bytes, from %s, data: %s\n", strlen(buf),
//				n, hostaddrp, buf);

		if (n > 0) {
			res = process_message(buf, msg_back);
		}

		/* send result back to the client */
		if (res > 0) {
			n = sendto(sockfd, msg_back, strlen(msg_back), 0,
					(struct sockaddr *) &clientaddr, clientlen);
			if (n < 0)
				error("ERROR in sendto");
		}

	}

	pthread_exit(NULL);
}

void hexDump(char *desc, void *addr, int len) {
	int i;
	unsigned char buff[17];
	unsigned char *pc = (unsigned char*) addr;

	// Output description if given.
	if (desc != NULL)
		printf("%s:\n", desc);

	if (len == 0) {
		printf("  ZERO LENGTH\n");
		return;
	}
	if (len < 0) {
		printf("  NEGATIVE LENGTH: %i\n", len);
		return;
	}

	// Process every byte in the data.
	for (i = 0; i < len; i++) {
		// Multiple of 16 means new line (with line offset).

		if ((i % 16) == 0) {
			// Just don't print ASCII for the zeroth line.
			if (i != 0)
				printf("  %s\n", buff);

			// Output the offset.
			printf("  %04x ", i);
		}

		// Now the hex code for the specific character.
		printf(" %02x", pc[i]);

		// And store a printable ASCII character for later.
		if ((pc[i] < 0x20) || (pc[i] > 0x7e))
			buff[i % 16] = '.';
		else
			buff[i % 16] = pc[i];
		buff[(i % 16) + 1] = '\0';
	}

	// Pad out last line if not exactly 16 characters.
	while ((i % 16) != 0) {
		printf("   ");
		i++;
	}

	// And print the final ASCII bit.
	printf("  %s\n", buff);
}

/*
 * Set current location as it is replayed by the GPS simulator.
 * This method should be called by the main gpssim thread to
 * reflect currently replayed location. Retrieval fo the location
 * is implemented using CUR_LOC message processing.
 * It is OK to have in not thread safe
 */
void set_cur_location(double llh[]) {
	//printf("set_cur_location %lf, %lf, %lf:\n", llh[0], llh[1], llh[2]);
	cur_loc.llh[0] = llh[0];
	cur_loc.llh[1] = llh[1];
	cur_loc.llh[2] = llh[2];
}
