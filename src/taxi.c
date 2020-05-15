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

/** structure pour passer les arguments à la fonction #taxi. */
struct arg_taxi {
	int id;							/**< identifiant */
	int fifo;						/**< descripteur de fichier du pipe nommé */
	pthread_mutex_t *mutex_fifo;	/**< assure l'exclusion mutuelle lors des accès au pipe nommé */
	sem_t *sem_arg;					/**< donne suffisamment de temps à un thread pour qu'il récupère les arguments */
};

/** variable booléenne pour signaler la fin du programme.
 * Quand cette variable vaut faux, les threads continuent à s'exécuter.
 * Si elle vaut vrai, les threads se terminent
 */
int termine = 0;

void creer_taxis(const char *nom_fifo) {
	int i, fifo;
	pthread_t pthread_taxi[NB_TAXIS];
	struct arg_taxi arg;
	sem_t sem_arg;
	pthread_mutex_t mutex;

	if (signal(SIGUSR1, terminer) == SIG_ERR) {
		perror("signal SIGUSR1");
		exit(EXIT_FAILURE);
	}
	fifo = open(nom_fifo, O_RDONLY);

	/* création des threads taxi */
	sem_init(&sem_arg, 0, 1);
	pthread_mutex_init(&mutex, NULL);
	for (i = 0; i<NB_TAXIS; i++) {
		sem_wait(&sem_arg);
		arg.id = i;
		arg.fifo = fifo;
		arg.mutex_fifo = &mutex;
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
	pthread_mutex_destroy(&mutex);
}

void terminer(int sig) {
	termine = 1;
}

void *taxi(void *arg) {
	int id, fifo;
	passager_t p;
	char debut_mess[15];
	pthread_mutex_t *mutex;
	struct arg_taxi *a;

	a = arg;
	id = a->id;
	fifo = a->fifo;
	mutex = a->mutex_fifo;
	/* récupération des arguments terminée */
	sem_post(a->sem_arg);

	snprintf(debut_mess, 15, "taxi %d : ", id);
	while (!termine) {
		pthread_mutex_lock(mutex);
		/* !termine sert à éviter le blocage infini
		 * dans read à la fin du programme */
		if (!termine && read(fifo, &p, sizeof(passager_t)) == -1) {
			/* le cas où l'erreur n'est pas liée au signal reçu */
			if (errno != EINTR) {
				perror("taxi read passager");
				exit(EXIT_FAILURE);
			}
		}
		pthread_mutex_unlock(mutex);

		usleep(10);
		printf("%spassager %ld est rendu a la station %d\n",
				debut_mess, p.id, p.station_arrivee);
	}

	pthread_exit(NULL);
}

