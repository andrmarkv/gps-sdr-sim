#ifndef PATH_GENERATOR_H
#define PATH_GENERATOR_H

/*
 * Linked list of coordinates forming a motion path
 */
typedef struct motion {
    int* xyz[3];
    struct motion *next;
} t_motion;

/*
 * Linked list of paths structures forming chain of motion paths
 */
typedef struct motions_list {
	t_motion* motion;
    struct motions_list *next;
} t_motions_list;

void* start_udp_server(void *arg);
t_motion* get_next_motion_path();

#endif
