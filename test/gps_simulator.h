#ifndef GPS_SIMULATOR_H
#define GPS_SIMULATOR_H

/*
 * Structure to hold all configuration parameters
 * of the gps simulation
 */
typedef struct sim_params {
	double samp_freq;
	char outfile[MAX_CHAR];
	char navfile[MAX_CHAR];
	datetime_t t0;
	gpstime_t g0;
	int verb;
} t_sim_params;

/*
 * Structure to hold all parameters necessary for
 * gps sim file generation
 */
typedef struct sim_context {
	gpstime_t grx;
	int iumd; // position index
	int numd; // number of positions
	channel_t chan[]; // Pointer to the broadcast channels
	ephem_t eph[]; // Pointer to loaded ephemerides file
	int ieph; //Index if the current satellilte
	ionoutc_t *ionoutc;
	double elvmask = 0.0; // in degree (this value is always 0 for now
} t_sim_context;

void* run_simulation(void *arg);

#endif
