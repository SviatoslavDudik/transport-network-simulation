/** @file */
#include "transport.h"
#include "constantes.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

/** structure pour passer les arguments à la fonction #vehicule(void *arg). */
struct arg_vehicule {
	liste_t *trajet;				/**< liste des stations qui composent le
									  trajet du véhicule */
	liste_t **stations;				/**< liste des toutes les stations */
	sem_t *sem_vehicule;			/**< sémaphore qui sera débloqué par le
									  vérificateur lors du rendez-vous
									  bilatéral, un de #arg_verif.sem_vehicule */
	sem_t *sem_verif;				/**< sémaphore débloquant le vérificateur
									  lors du rendez-vous bilatéral avec le
									  vérificateur, un de #arg_verif.sem_verif */
	pthread_mutex_t *mutex_corresp;	/**< mutex assurant l'accès aux stations
									  qui possèdent une correspondance */
	int est_metro;					/**< booléen valant vrai ssi le véhicule
									  est un métro */
	int *termine;					/**< pointeur vers un booléen commun à tous
									  les threads valant vrai si le programme
									  est terminé */
};

/** structure pour passer les arguments à la fonction #verificateur(void *arg).
 */
struct arg_verif {
	liste_t **stations;				/**< liste des toutes les stations */
	sem_t *sem_verif;				/**< tableau de sémaphores qui seront
									  débloqués par les véhicules lors du
									  rendez-vous bilatéral, composé de tous
									  les #arg_vehicule.sem_verif */
	sem_t *sem_vehicule;			/**< tableau de sémaphores débloquant les
									  véhicules lors du rendez-vous bilatéral,
									  contient tous les
									  #arg_vehicule.sem_vehicule */
	int fifo;						/**< descripteur de fichier du pipe nommé */
	int nb_vehic;					/**< nombre de véhicles (bus + métro) */
	int *termine;					/**< pointeur vers un booléen commun à tous
									  les threads valant vrai si le programme
									  est terminé */
};

long gerer_transport(const char *nom_fifo, liste_t **stations, liste_t *trajets, int nb_bus) {
	pthread_mutex_t *mutex_stations;
	pthread_t *pthread_vehicule, *pthread_verif;
	sem_t *sem_vehicule, *sem_verif;
	int i, fifo, *termine;
	long sum;

	sem_vehicule = malloc(sizeof(sem_t)*trajets->taille);
	sem_verif = malloc(sizeof(sem_t)*trajets->taille);
	mutex_stations = malloc(sizeof(pthread_mutex_t));
	termine = malloc(sizeof(int));
	if (!sem_vehicule || !sem_verif || !mutex_stations || !termine) {
		fprintf(stderr, "Erreur allocation creer transport\n");
		exit(EXIT_FAILURE);
	}

	for (i = 0; i<trajets->taille; i++) {
		sem_init(sem_vehicule+i, 0, 0);
		sem_init(sem_verif+i, 0, 0);
	}

	/* créations des transports */
	pthread_mutex_init(mutex_stations, NULL);
	*termine = 0;
	pthread_vehicule = creer_transport(stations, trajets, nb_bus, sem_vehicule,
			sem_verif, mutex_stations, termine);

	/* création du vérificateur */
	fifo = open(nom_fifo, O_WRONLY);
	pthread_verif = creer_verificateur(stations, sem_vehicule, sem_verif, fifo,
			trajets->taille, termine);

	/* join des threads et récupération de la valeur de retour */
	sum = calculer_revenu(pthread_vehicule, pthread_verif, trajets->taille);

	for (i = 0; i<trajets->taille; i++) {
		sem_destroy(sem_vehicule+i);
		sem_destroy(sem_verif+i);
	}
	pthread_mutex_destroy(mutex_stations);
	free(pthread_vehicule);
	free(pthread_verif);
	free(sem_vehicule);
	free(sem_verif);
	free(mutex_stations);
	free(termine);
	close(fifo);
	return sum;
}

pthread_t *creer_transport(liste_t **stations, liste_t *trajets, int nb_bus,
		sem_t *sem_vehicule, sem_t *sem_verif, pthread_mutex_t *mutex, int *termine) {
	struct maillon *m;
	pthread_t *pthread_id;
	struct arg_vehicule *arg;
	int i;

	pthread_id = malloc(sizeof(pthread_t)*trajets->taille);
	if (pthread_id == NULL) {
		fprintf(stderr, "Erreur allocation pthread transport\n");
		exit(EXIT_FAILURE);
	}

	i = 0;
	for (m = trajets->tete; m != NULL; m = m->suivant) {
		if ((arg = malloc(sizeof(struct arg_vehicule))) == NULL) {
			fprintf(stderr, "Erreur allocation argument transport\n");
			exit(EXIT_FAILURE);
		}
		arg->trajet = m->donnee;
		arg->stations = stations;
		arg->sem_vehicule = sem_vehicule+i;
		arg->sem_verif = sem_verif+i;
		arg->mutex_corresp = mutex;
		arg->est_metro = i>=nb_bus;
		arg->termine = termine;
		if (pthread_create(pthread_id+i, NULL, vehicule, arg) != 0) {
			fprintf(stderr, "Erreur creation pthread véhicule\n");
			exit(EXIT_FAILURE);
		}
		i++;
	}

	return pthread_id;
}

pthread_t *creer_verificateur(liste_t **stations, sem_t *sem_vehicule,
		sem_t *sem_verif, int fifo, int nb_vehicules, int *termine) {
	struct arg_verif *arg;
	pthread_t *pthread;

	arg = malloc(sizeof(struct arg_verif));
	pthread = malloc(sizeof(pthread_t));
	if (arg == NULL || pthread == NULL) {
		fprintf(stderr, "Erreur creation pthread vérificateur\n");
		exit(EXIT_FAILURE);
	}

	arg->stations = stations;
	arg->sem_verif = sem_verif;
	arg->sem_vehicule = sem_vehicule;
	arg->fifo = fifo;
	arg->nb_vehic = nb_vehicules;
	arg->termine = termine;
	if (pthread_create(pthread, NULL, verificateur, arg) != 0) {
		fprintf(stderr, "Erreur creation pthread vérificateur\n");
		exit(EXIT_FAILURE);
	}

	return pthread;
}

long calculer_revenu(pthread_t *pthread_vehicule, pthread_t *pthread_verif, int taille) {
	int i;
	long sum;
	long *tmp;

	sum = 0;
	/* récupération de la valeur de retour du vérificateur */
	if (pthread_join(*pthread_verif, (void**)&tmp) != 0) {
		fprintf(stderr, "Erreur pthread join vérificateur\n");
		exit(EXIT_FAILURE);
	}
	sum += *tmp;
	free(tmp);

	/* ajout des valeur de retour des transports */
	for (i = 0; i<taille; i++) {
		if (pthread_join(pthread_vehicule[i],(void**)&tmp) != 0) {
			fprintf(stderr, "Erreur pthread join véhicule\n");
			exit(EXIT_FAILURE);
		}
		sum += *tmp;
		free(tmp);
	}

	return sum;
}

void *vehicule(void *arg) {
	int i, dec;
	long *sum;
	union int_pvoid u;
	struct maillon *p, *suiv;
	liste_t *passagers;
	struct arg_vehicule *a;
	a = arg;

	if ((sum = malloc(sizeof(long))) == NULL) {
		fprintf(stderr, "Erreur allocation sum véhicule\n");
		exit(EXIT_FAILURE);
	}
	passagers = liste_init();
	p = a->trajet->tete;
	/* decalage de la station intiale pour un metro */
	if (a->est_metro) {
		dec = a->trajet->taille/2 - 1;
		for (i = 0; i<dec; i++)
			p = p->suivant;
	}
	while (!(*(a->termine)) || passagers->taille>0) {
		/* p pointe sur la station du a->trajet où on se trouve */
		while (p != NULL) {
			u.p = p->donnee;
			*sum += debarquer(passagers, a->stations, u.i, a->est_metro, a->mutex_corresp);
			/* recherche de la direction */
			if (a->est_metro) {
				suiv = p->suivant ? p->suivant : a->trajet->tete;
				if (suiv->donnee < p->donnee) {
					u.i = STATION_DIR_DECR(u.i);
				} else {
					u.i = STATION_DIR_CROI(u.i);
				}
			}
			embarquer(passagers, a->stations, u.i, a->est_metro, a->mutex_corresp);
			/* rendez-vous bilatéral avec le vérificateur */
			sem_post(a->sem_verif);
			sem_wait(a->sem_vehicule);
			p = p->suivant;
		}

		p = a->trajet->tete;
	}

	liste_detruire(passagers);
	free(a);
	pthread_exit(sum);
}

int embarquer(liste_t *passagers, liste_t **stations, int indice_st, int metro, pthread_mutex_t *mutex_corresp) {
	passager_t *p;
	char debut_mess[10];
	int num_station, capacite;
	int nb_pass;
	if (metro) {
		num_station = NUM_STATION(indice_st);
		capacite = CAPACITE_METRO;
		strcpy(debut_mess, "metro : ");
	} else {
		num_station = indice_st;
		capacite= CAPACITE_BUS;
		strcpy(debut_mess, "bus : ");
	}
	nb_pass = 0;

	if (correspondance[num_station] >= 0)
		pthread_mutex_lock(mutex_corresp);
	/* remplissage du véhicule */
	while (stations[indice_st]->taille > 0 && passagers->taille < capacite) {
		p = liste_rem_tete(stations[indice_st]);
		liste_add_queue(passagers, p);
		printf("%sembarque le passager %ld\n", debut_mess, p->id);
		nb_pass++;
	}
	if (correspondance[num_station] >= 0)
		pthread_mutex_unlock(mutex_corresp);
	return nb_pass;
}

int debarquer(liste_t *passagers, liste_t **stations, int num_station, int metro, pthread_mutex_t *mutex_corresp) {
	struct maillon *prec, *m;
	passager_t *p;
	char debut_mess[10];
	int corr, nb_pass;
	if (metro) {
		strcpy(debut_mess, "metro : ");
	} else {
		strcpy(debut_mess, "bus : ");
	}

	nb_pass = 0;
	/* parcours de la liste */
	/* prec précède toujours m */
	prec = NULL;
	m = passagers->tete;
	while (m != NULL) {
		p = m->donnee;
		corr = correspondance[num_station];
		if (p->station_arrivee == num_station) {
			nb_pass++;
			/* on prend m->suivant avant de supprimer m */
			m = m->suivant;
			liste_rem_suivant(passagers, prec);
			printf("%sdebarque le passager %ld\n", debut_mess, p->id);
			free(p);
		} else if (p->transfert && corr >= 0) {
			nb_pass++;
			/* on prend m->suivant avant de supprimer m */
			m = m->suivant;
			liste_rem_suivant(passagers, prec);
			printf("%sdebarque le passager %ld\n", debut_mess, p->id);

			if (p->station_arrivee == corr) {
				/* on compte le transfert */
				nb_pass++;
				printf("%stranfert passager %ld vers station %d\n", debut_mess,
						p->id, correspondance[num_station]);
				continue;
			}

			if (p->station_arrivee < corr) {
				corr = STATION_DIR_DECR(corr);
			} else {
				corr = STATION_DIR_CROI(corr);
			}
			p->transfert = 0;
			pthread_mutex_lock(mutex_corresp);
			liste_add_queue(stations[corr], p);
			pthread_mutex_unlock(mutex_corresp);
			printf("%stranfert passager %ld vers station %d\n", debut_mess,
					p->id, correspondance[num_station]);
		} else {
			prec = m;
			m = m->suivant;
		}
	}
	return nb_pass;
}

void *verificateur(void *arg) {
	int i, j, stations_vides;
	long *sum;
	struct arg_verif *a;
	a = arg;

	if ((sum = malloc(sizeof(long))) == NULL) {
		fprintf(stderr, "Erreur allocation sum véhicule\n");
		exit(EXIT_FAILURE);
	}
	*sum = 0;
	stations_vides = 0;
	while (!stations_vides) {
		stations_vides = 1;
		for (i = 0; i<NB_STATIONS_TOTAL; i++) {
			/* attente pour que chaque véhicule termine son cycle*/
			for (j = 0; j<a->nb_vehic; j++)
				sem_wait(a->sem_verif+j);

			if (verifier_passagers(a->stations, a->fifo, sum) == 0)
				stations_vides = 0;

			/* lancement du cycle suivant de chaque véhicule */
			for (j = 0; j<a->nb_vehic; j++)
				sem_post(a->sem_vehicule+j);
		}
	}
	/* terminaison des véhicules */
	*(a->termine) = 1;
	/* débloquer les véhicules pour qu'ils puissent terminer correctement */
	for (i = 0; i<2*NB_STATIONS_TOTAL; i++)
		for (j = 0; j<a->nb_vehic; j++)
			sem_post(a->sem_vehicule+j);

	free(a);
	pthread_exit(sum);
}

int verifier_passagers(liste_t **stations, int fifo, long *sum) {
	int i, stations_vides;
	struct maillon *m, *prec;
	passager_t *p;
	char debut_mess[] = "verificateur : ";
	stations_vides = 1;
	for (i = 0; i<NB_STATIONS_TOTAL; i++) {
		if (stations[i]->taille > 0)
			stations_vides = 0;

		/* parcours de la liste des stations */
		/* prec précède toujours m */
		prec = NULL;
		m = stations[i]->tete;
		while (m != NULL) {
			p = m->donnee;
			p->temps_ecoule += 1;
			if (p->temps_ecoule >= p->temps_att_max) {
				/* on prend m->suivant avant de supprimer m */
				m = m->suivant;
				liste_rem_suivant(stations[i], prec);
				printf("%stransfert du passager %ld vers le taxi\n", debut_mess, p->id);
				write(fifo, p, sizeof(passager_t));
				free(p);
				*sum += 3;
			} else {
				prec = m;
				m = m->suivant;
			}
		}
	}
	return stations_vides;
}

