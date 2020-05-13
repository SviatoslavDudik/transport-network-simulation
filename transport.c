#include "transport.h"

void creer_transport(const char *nom_fifo, liste_t **stations, liste_t *trajets, int nb_bus) {
	struct maillon *m;
	pthread_mutex_t *mutex_stations;
	struct arg_vehicule arg;
	pthread_t *pthread_vehicule, pthread_verif;
	sem_t *sem_arg, *sem_vehicule, *sem_verif;
	int i;
	pthread_vehicule = malloc(sizeof(pthread_t)*trajets->taille);
	sem_vehicule = malloc(sizeof(sem_t)*trajets->taille);
	sem_verif = malloc(sizeof(sem_t)*trajets->taille);
	sem_arg = malloc(sizeof(sem_t));
	mutex_stations = malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(mutex_stations, NULL);
	sem_init(sem_arg, 0, 1);
	i = 0;
	for (m = trajets->tete; m != NULL; m = m->suivant) {
		sem_wait(sem_arg);
		arg.trajet = m->donnee;
		arg.stations = stations;
		sem_init(sem_vehicule+i, 0, 0);
		arg.sem = sem_vehicule+i;
		sem_init(sem_verif+i, 0, 0);
		arg.sem_verif = sem_verif+i;
		arg.mutex_corresp = mutex_stations;
		arg.est_metro = i>=nb_bus;
		arg.sem_arg = sem_arg;
		if (pthread_create(pthread_vehicule+i, NULL, vehicule, &arg) != 0) {
			fprintf(stderr, "Erreur creation pthread vehicule");
			exit(EXIT_FAILURE);
		}
		i++;
	}
	free(pthread_vehicule);
	free(sem_vehicule);
	free(sem_verif);
	free(sem_arg);
	free(mutex_stations);
	pthread_mutex_destroy(mutex_stations);
}

void *vehicule(void *arg) {
	int termine, metro;
	union int_pvoid u;
	struct maillon *p, *suiv;
	liste_t *trajet, *passagers;
	liste_t **stations;
	sem_t *sem, *sem_verif;
	pthread_mutex_t *mutex_corresp;
	struct arg_vehicule *a;
	a = (struct arg_vehicule*) arg;

	/* récupération des arguments */
	trajet = a->trajet;
	stations = a->stations;
	sem = a->sem;
	sem_verif = a->sem_verif;
	mutex_corresp = a->mutex_corresp;
	metro = a->est_metro;
	/* fin de la récupération */
	sem_post(a->sem_arg);

	passagers = liste_init();
	termine = 0;
	while (!termine) {
		termine = 1;
		/* p pointe sur la station du trajet où on se trouve */
		for (p = trajet->tete; p != NULL; p = p->suivant) {
			u.p = p->donnee;
			if(debarquer(passagers, stations, u.i, metro, mutex_corresp))
				termine = 0;
			/* recherche de la direction */
			if (metro) {
				suiv = p->suivant ? p->suivant : trajet->tete;
				if (suiv->donnee < p->donnee) {
					u.i = STATION_DIR_DECR(u.i);
				} else {
					u.i = STATION_DIR_CROI(u.i);
				}
			}
			if (embarquer(passagers, stations, u.i, metro, mutex_corresp))
				termine = 0;
			/* rendez-vous bilatéral avec le vérificateur */
			sem_post(sem_verif);
			sem_wait(sem);
		}
	}
	pthread_exit(NULL);
}

int debarquer(liste_t *passagers, liste_t **stations, int indice_st, int metro, pthread_mutex_t *mutex_corresp) {
	struct maillon *prec, *m;
	passager_t *p;
	char debut_mess[10];
	int num_station, corr, nb_pass;
	if (metro) {
		num_station = NUM_STATION(indice_st);
		strcpy(debut_mess, "metro : ");
	} else {
		num_station = indice_st;
		strcpy(debut_mess, "bus : ");
	}
	nb_pass = 0;
	prec = NULL;
	for (m = passagers->tete; m != NULL; prec = m, m = m->suivant) {
		p = m->donnee;
		if (p->station_arrivee == num_station) {
			liste_rem_element(passagers, prec);
			printf("%s", debut_mess);
			printf("debarque le passager %ld\n", p->id);
			free(p);
			nb_pass++;
		}
		else if (p->transfert && (corr=correspondance[num_station]) >= 0) {
			liste_rem_element(passagers, prec);
			printf("%s", debut_mess);
			printf("debarque le passager %ld\n", p->id);
			if (p->station_arrivee < corr)
				corr = STATION_DIR_DECR(corr);
			else
				corr = STATION_DIR_CROI(corr);
			pthread_mutex_lock(mutex_corresp);
			liste_add_queue(stations[corr], p);
			pthread_mutex_unlock(mutex_corresp);
			printf("%s", debut_mess);
			printf("tranfert passager %ld vers station %d\n", p->id, num_station);
			nb_pass++;
		}
	}
	return nb_pass;
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
	while (stations[indice_st]->taille > 0 && passagers->taille < capacite) {
		p = liste_rem_tete(stations[indice_st]);
		liste_add_queue(passagers, p);
		printf("%s", debut_mess);
		printf("embarque le passager %ld\n", p->id);
		nb_pass++;
	}
	if (correspondance[num_station] >= 0)
		pthread_mutex_unlock(mutex_corresp);
	return nb_pass;
}

