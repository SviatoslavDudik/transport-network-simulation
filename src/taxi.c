/** @file */
#include "taxi.h"
#include "constantes.h"
#define _DEFAULT_SOURCE
#include <pthread.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

/** structure pour passer les arguments à la fonction #taxi(void *arg). */
struct arg_taxi {
	int id;							/**< identifiant */
	int fifo;						/**< descripteur de fichier du pipe nommé */
	sem_t *sem;						/**< sémaphore responsable du pipe nommé*/
};

void gerer_taxis(const char *nom_fifo) {
	int i, fifo;
	pthread_t pthread_taxi[NB_TAXIS];
	struct arg_taxi arg[NB_TAXIS];
	/* semaphore responsable du pipe nommé */
	sem_t sem;

	fifo = open(nom_fifo, O_RDONLY);

	sem_init(&sem, 0, 1);
	/* création des threads taxi */
	for (i = 0; i<NB_TAXIS; i++) {
		arg[i].id = i;
		arg[i].fifo = fifo;
		arg[i].sem = &sem;
		if (pthread_create(pthread_taxi+i, NULL, taxi, arg+i) != 0) {
			fprintf(stderr, "Erreur creation pthread taxi\n");
			exit(EXIT_FAILURE);
		}
	}

	/* join les threads créer plus haut */
	for (i = 0; i<NB_TAXIS; i++) {
		if (pthread_join(pthread_taxi[i], NULL) != 0) {
			fprintf(stderr, "Erreur join pthread taxi\n");
			exit(EXIT_FAILURE);
		}
	}
	sem_destroy(&sem);
}

void *taxi(void *arg) {
	passager_t p;
	char debut_mess[15];
	struct arg_taxi *a;
	a = arg;

	snprintf(debut_mess, 15, "taxi %d : ", a->id);
	while (1) {
		sem_wait(a->sem);
		/* si termine est vrai, alors le thread sera bloqué dans read */
		if (read(a->fifo, &p, sizeof(passager_t)) != sizeof(passager_t)) {
			/* le cas où l'erreur n'est pas liée au signal reçu */
			if (errno) {
				perror("taxi read passager");
				exit(EXIT_FAILURE);
			}
			break;
		}
		sem_post(a->sem);

		usleep(10);
		printf("%spassager %ld est rendu a la station %d\n",
				debut_mess, p.id, p.station_arrivee);
	}

	pthread_exit(NULL);
}

