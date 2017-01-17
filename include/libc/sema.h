/*
 * Semaphores
 * Copyright (c) 2003, Jeffrey K. Hollingsworth <hollings@cs.umd.edu>
 * Copyright (c) 2004, David H. Hovemeyer <daveho@cs.umd.edu>
 * $Revision: 1.7 $
 * 
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "COPYING".
 */

#ifndef SEMA_H
#define SEMA_H

#include <geekos/ktypes.h>              /* We add it */

#define MAX_SEMAPHORE_NAME_LENGHT 25    /* We add it */
#define TOTAL_SEMAPHORES 20             /* We add it */

typedef struct {                        /* We add it */
    char name[MAX_SEMAPHORE_NAME_LENGHT];
    int value;
	int id;
	int num_ref;                       /* Number of processes have referenced this semaphore. */
	bool exist;                         /* Since we didn't use linked list, we need this variable.*/
    struct Thread_Queue *waitQueue;
    
	// I think I need a list so I can use it to check if the processor
	// has permission to use the semaphore.
	// It's just that when a processor attempt to create a semaphore then 
	// I add the processor to the authorized list of processors to use the semaphore.
} semaphore;

semaphore semaphores[TOTAL_SEMAPHORES]; /* We add it */



int Create_Semaphore(const char *name, int ival);
int P(int sem);
int V(int sem);
int Destroy_Semaphore(int sem);
#endif  /* SEMA_H */
