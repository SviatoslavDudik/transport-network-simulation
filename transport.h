#ifndef _TRANSPORT_H_
#define _TRANSPORT_H_

#include "constantes.h"
#include "liste.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

struct arg_vehicule {
	liste_t *trajet;
	liste_t **stations;
	sem_t *sem;
	sem_t *sem_verif;
	sem_t *sem_arg;
	pthread_mutex_t *mutex_corresp;
	int est_metro;
	int *termine;
};

struct arg_verif {
	liste_t **stations;
	sem_t *sem_verif;
	sem_t *sem_vehicule;
	int fifo;
	int nb_vehic;
	int *termine;
};

long creer_transport(const char*, liste_t**, liste_t*, int);
void *vehicule(void*);
int debarquer(liste_t*, liste_t **, int, int, pthread_mutex_t *);
int embarquer(liste_t*, liste_t **, int, int, pthread_mutex_t *);
void *verificateur(void*);
int verifier_passagers(liste_t**, int, long*);

#endif
