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
	sem_t *sem_arg;					/**< donne suffisamment de temps à un thread pour qu'il récupère les arguments */
};

/** variable booléenne pour signaler la fin du programme.
 * Quand cette variable vaut faux, les threads continuent à s'exécuter.
 * Si elle vaut vrai, les threads se terminent.
 */
int termine = 0;
/** semaphore responsable du pipe nommé et de #termine.
 * Le semaphore doit être testé avant d'accéder au pipe nommé.
 */
sem_t sem;

void gerer_taxis(const char *nom_fifo) {
	int i, fifo;
	pthread_t pthread_taxi[NB_TAXIS];
	struct arg_taxi arg;
	sem_t sem_arg;

	if (signal(SIGUSR1, terminer) == SIG_ERR) {
		perror("signal SIGUSR1");
		exit(EXIT_FAILURE);
	}
	fifo = open(nom_fifo, O_RDONLY);

	/* création des threads taxi */
	sem_init(&sem_arg, 0, 1);
	sem_init(&sem, 0, 1);
	for (i = 0; i<NB_TAXIS; i++) {
		sem_wait(&sem_arg);
		arg.id = i;
		arg.fifo = fifo;
		arg.sem_arg = &sem_arg;
		if (pthread_create(pthread_taxi+i, NULL, taxi, &arg) != 0) {
			fprintf(stderr, "Erreur creation pthread taxi\n");
			exit(EXIT_FAILURE);
		}
	}
	sem_destroy(&sem_arg);

	/* join les threads créer plus haut */
	for (i = 0; i<NB_TAXIS; i++) {
		if (pthread_join(pthread_taxi[i], NULL) != 0) {
			fprintf(stderr, "Erreur join pthread taxi\n");
			exit(EXIT_FAILURE);
		}
	}
	sem_destroy(&sem);
}

void terminer(int sig) {
	int i;
	termine = 1;
	/* debloquer les threads */
	for (i = 0; i<NB_TAXIS; i++)
		sem_post(&sem);
}

void *taxi(void *arg) {
	int id, fifo;
	passager_t p;
	char debut_mess[15];
	struct arg_taxi *a;

	a = arg;
	id = a->id;
	fifo = a->fifo;
	/* récupération des arguments terminée */
	sem_post(a->sem_arg);

	snprintf(debut_mess, 15, "taxi %d : ", id);
	while (!termine) {
		sem_wait(&sem);
		/* si termine est vrai, alors le thread sera bloqué dans read */
		if (termine || read(fifo, &p, sizeof(passager_t)) != sizeof(passager_t)) {
			/* le cas où l'erreur n'est pas liée au signal reçu */
			if (errno) {
				perror("taxi read passager");
				exit(EXIT_FAILURE);
			}
			break;
		}
		sem_post(&sem);

		usleep(10);
		printf("%spassager %ld est rendu a la station %d\n",
				debut_mess, p.id, p.station_arrivee);
	}

	pthread_exit(NULL);
}

