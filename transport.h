#ifndef _TRANSPORT_H_
#define _TRANSPORT_H_

#include "constantes.h"
#include "liste.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>

struct arg_vehicule {
	liste_t *trajet;
	liste_t **stations;
	sem_t *sem;
	sem_t *sem_verif;
	sem_t *sem_arg;
	pthread_mutex_t *mutex_corresp;
	int est_metro;
};

void creer_transport(const char*, liste_t**, liste_t*, int);
void *vehicule(void*);
int debarquer(liste_t*, liste_t **, int, int, pthread_mutex_t *);
int embarquer(liste_t*, liste_t **, int, int, pthread_mutex_t *);

#endif
