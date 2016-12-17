#define _CRT_SECURE_NO_DEPRECATE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <omp.h>
#ifdef _WIN32
#include "getopt.h"
#else
#include <unistd.h>
#endif
#include <pthread.h>
#include "gpssim.h"
#include "path_generator.h"

void usage(void)
{
	printf("Usage: gps-sdr-sim [options]\n"
		"Options:\n"
		"  -e <gps_nav>     RINEX navigation file for GPS ephemerides (required)\n"
		"  -t <date,time>   Scenario start time YYYY/MM/DD,hh:mm:ss\n"
		"  -T <date,time>   Overwrite TOC and TOE to scenario start time\n"
		"  -o <output>      I/Q sampling data file (default: gpssim.bin)\n"
		"  -s <frequency>   Sampling frequency [Hz] (default: 2600000)\n"
		"  -v               Show details about simulated channels\n",
		((double)USER_MOTION_SIZE)/10.0);

	return;
}

int main(int argc, char *argv[])
{
	char navfile[MAX_CHAR];
	char outfile[MAX_CHAR];
	double samp_freq;
	gpstime_t g0;
	int verb;
	ionoutc_t ionoutc;

	////////////////////////////////////////////////////////////
	// Read options
	////////////////////////////////////////////////////////////

	// Default options
	strcpy(outfile, "gpssim.bin");
	samp_freq = 2.6e6;
	data_format = SC16;
	g0.week = -1; // Invalid start time
	verb = FALSE;
	ionoutc.enable = TRUE;

	if (argc<3)
	{
		usage();
		exit(1);
	}

	while ((result=getopt(argc,argv,"e:o:s:T:t:v"))!=-1)
	{
		switch (result)
		{
		case 'e':
			strcpy(navfile, optarg);
			break;
		case 'o':
			strcpy(outfile, optarg);
			break;
		case 's':
			samp_freq = atof(optarg);
			if (samp_freq<1.0e6)
			{
				printf("ERROR: Invalid sampling frequency.\n");
				exit(1);
			}
			break;
		case 'T':
			timeoverwrite = TRUE;
			if (strncmp(optarg, "now", 3)==0)
			{
				time_t timer;
				struct tm *gmt;
				
				time(&timer);
				gmt = gmtime(&timer);

				t0.y = gmt->tm_year+1900;
				t0.m = gmt->tm_mon+1;
				t0.d = gmt->tm_mday;
				t0.hh = gmt->tm_hour;
				t0.mm = gmt->tm_min;
				t0.sec = (double)gmt->tm_sec;

				date2gps(&t0, &g0);

				break;
			}
		case 't':
			sscanf(optarg, "%d/%d/%d,%d:%d:%lf", &t0.y, &t0.m, &t0.d, &t0.hh, &t0.mm, &t0.sec);
			if (t0.y<=1980 || t0.m<1 || t0.m>12 || t0.d<1 || t0.d>31 ||
				t0.hh<0 || t0.hh>23 || t0.mm<0 || t0.mm>59 || t0.sec<0.0 || t0.sec>=60.0)
			{
				printf("ERROR: Invalid date and time.\n");
				exit(1);
			}
			t0.sec = floor(t0.sec);
			date2gps(&t0, &g0);
		case 'v':
			verb = TRUE;
			break;
		case ':':
		case '?':
			usage();
			exit(1);
		default:
			break;
		}
	}

	// Buffer size	
	samp_freq = floor(samp_freq/10.0);
	samp_freq *= 10.0;


	////////////////////////////////////////////////////////////
	// Starting UDP thread that should receive path messages
	////////////////////////////////////////////////////////////
	pthread_t tid;
	int err = pthread_create(&tid, NULL, &start_udp_server, NULL);
	if (err != 0)
		printf("\nCan't create UDP Server Thread :[%s]", strerror(err));
	else
		printf("\nUDP Server Thread created successfully\n");


	////////////////////////////////////////////////////////////
	// Starting Simulation thread - write simulated data to file
	////////////////////////////////////////////////////////////
	(void *)&args
	pthread_t tid_sim;
	err = pthread_create(&tid_sim, NULL, &run_simulation, NULL);
	if (err != 0)
		printf("\nCan't create GPS Simulation Thread :[%s]", strerror(err));
	else
		printf("\nGPS Simulation Thread created successfully\n");


	////////////////////////////////////////////////////////////
	// Receiver position NEW APPROACH
	////////////////////////////////////////////////////////////
	printf("All ready waiting for receiver position!\n");
	t_motion *motion;
	int j = 0;
	while (1) {
		motion = get_next_motion_path();

		if(!motion){
			usleep(100);

			j++;
			if (j % 10000 == 0) {
				printf("No motion_path for simulation, waiting...\n");
			}
		} else {
			printf("Got initial path, staring GPS simulation...\n");
			del_motion_path(motion);
			print_motion(motion);
			printf("Finished simulating, removed motion path\n");
		}
	}

	return(0);
}
