#ifndef _TRANSPORT_H_
#define _TRANSPORT_H_

#include "liste.h"
#include <pthread.h>
#include <semaphore.h>

long gerer_transport(const char *nom_fifo, liste_t **stations, liste_t *trajets, int nb_bus);

pthread_t *creer_transport(liste_t **stations, liste_t *trajets, int nb_bus, sem_t *sem_vehicule, sem_t *sem_verif, pthread_mutex_t *mutex, int *termine);

pthread_t *creer_verificateur(liste_t **stations, sem_t *sem_vehicule, sem_t *sem_verif, int fifo, int nb_vehicules, int *termine);

long calculer_revenu(pthread_t *pthread_vehicule, pthread_t * pthread_verif, int taille);

void *vehicule(void *arg);

int embarquer(liste_t *passagers, liste_t **stations, int indice_st, int metro, pthread_mutex_t *mutex_corresp);

int debarquer(liste_t *passagers, liste_t **stations, int indice_station, int metro, pthread_mutex_t *mutex_corresp);

void *verificateur(void *arg);

int verifier_passagers(liste_t **stations, int fifo, long *sum);
#endif
