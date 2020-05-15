#include "transport.h"

long creer_transport(const char *nom_fifo, liste_t **stations, liste_t *trajets, int nb_bus) {
	struct maillon *m;
	pthread_mutex_t *mutex_stations;
	struct arg_vehicule arg;
	struct arg_verif arg_verif;
	pthread_t *pthread_vehicule, pthread_verif;
	sem_t *sem_arg, *sem_vehicule, *sem_verif;
	int i, fifo, *termine;
	long sum, *tmp;

	pthread_vehicule = malloc(sizeof(pthread_t)*trajets->taille);
	sem_vehicule = malloc(sizeof(sem_t)*trajets->taille);
	sem_verif = malloc(sizeof(sem_t)*trajets->taille);
	sem_arg = malloc(sizeof(sem_t));
	mutex_stations = malloc(sizeof(pthread_mutex_t));
	termine = malloc(sizeof(int));
	if (!pthread_vehicule || !sem_vehicule || !sem_verif
			|| !sem_arg || !mutex_stations || ! termine) {
		fprintf(stderr, "Erreur allocation creer transport\n");
		exit(EXIT_FAILURE);
	}
	pthread_mutex_init(mutex_stations, NULL);
	sem_init(sem_arg, 0, 1);
	*termine = 0;
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
		arg.termine = termine;
		arg.sem_arg = sem_arg;
		if (pthread_create(pthread_vehicule+i, NULL, vehicule, &arg) != 0) {
			fprintf(stderr, "Erreur creation pthread véhicule\n");
			exit(EXIT_FAILURE);
		}
		i++;
	}
	sem_destroy(sem_arg);

	fifo = open(nom_fifo, O_WRONLY);
	arg_verif.stations = stations;
	arg_verif.sem_verif = sem_verif;
	arg_verif.sem_vehicule = sem_vehicule;
	arg_verif.fifo = fifo;
	arg_verif.nb_vehic = trajets->taille;
	arg_verif.termine = termine;
	if (pthread_create(&pthread_verif, NULL, verificateur, &arg_verif) != 0) {
		fprintf(stderr, "Erreur creation pthread vérificateur\n");
		exit(EXIT_FAILURE);
	}

	sum = 0;
	if (pthread_join(pthread_verif, (void**)&tmp) != 0) {
		fprintf(stderr, "Erreur pthread join vérificateur\n");
		exit(EXIT_FAILURE);
	}
	sum += *tmp;
	free(tmp);
	for (i = 0; i<trajets->taille; i++) {
		if (pthread_join(pthread_vehicule[i],(void**)&tmp) != 0) {
			fprintf(stderr, "Erreur pthread join véhicule\n");
			exit(EXIT_FAILURE);
		}
		sum += *tmp;
		free(tmp);
		sem_destroy(sem_vehicule+i);
		sem_destroy(sem_verif+i);
	}

	pthread_mutex_destroy(mutex_stations);
	free(pthread_vehicule);
	free(sem_vehicule);
	free(sem_verif);
	free(sem_arg);
	free(mutex_stations);
	free(termine);
	close(fifo);
	return sum;
}

void *vehicule(void *arg) {
	int i, dec, metro, *termine;
	long *sum;
	union int_pvoid u;
	struct maillon *p, *suiv;
	liste_t *trajet, *passagers;
	liste_t **stations;
	sem_t *sem, *sem_verif;
	pthread_mutex_t *mutex_corresp;
	struct arg_vehicule *a;
	a = arg;

	/* récupération des arguments */
	trajet = a->trajet;
	stations = a->stations;
	sem = a->sem;
	sem_verif = a->sem_verif;
	mutex_corresp = a->mutex_corresp;
	metro = a->est_metro;
	termine = a->termine;
	/* fin de la récupération */
	sem_post(a->sem_arg);

	if ((sum = malloc(sizeof(long))) == NULL) {
		fprintf(stderr, "Erreur allocation sum véhicule\n");
		exit(EXIT_FAILURE);
	}
	passagers = liste_init();
	p = trajet->tete;
	/* decalage de la station intiale pour un metro */
	if (metro) {
		dec = trajet->taille/2 - 1;
		for (i = 0; i<dec; i++)
			p = p->suivant;
	}
	while (!(*termine) || passagers->taille>0) {
		/* p pointe sur la station du trajet où on se trouve */
		while (p != NULL) {
			u.p = p->donnee;
			*sum += debarquer(passagers, stations, u.i, metro, mutex_corresp);
			/* recherche de la direction */
			if (metro) {
				suiv = p->suivant ? p->suivant : trajet->tete;
				if (suiv->donnee < p->donnee) {
					u.i = STATION_DIR_DECR(u.i);
				} else {
					u.i = STATION_DIR_CROI(u.i);
				}
			}
			embarquer(passagers, stations, u.i, metro, mutex_corresp);
			/* rendez-vous bilatéral avec le vérificateur */
			sem_post(sem_verif);
			sem_wait(sem);
			p = p->suivant;
		}

		p = trajet->tete;
	}

	liste_detruire(passagers);
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
	int corr, num_station_corr, nb_pass;
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
			num_station_corr = corr;
			if (p->station_arrivee == corr) {
				/* on compte le transfert */
				nb_pass++;
				printf("%stranfert passager %ld vers station %d\n", debut_mess, p->id, num_station_corr);
				continue;
			}
			if (p->station_arrivee < corr) {
				corr = STATION_DIR_DECR(num_station_corr);
			} else {
				corr = STATION_DIR_CROI(num_station_corr);
			}
			p->transfert = 0;
			pthread_mutex_lock(mutex_corresp);
			liste_add_queue(stations[corr], p);
			pthread_mutex_unlock(mutex_corresp);
			printf("%stranfert passager %ld vers station %d\n", debut_mess, p->id, num_station_corr);
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
	for (i = 0; i<NB_STATIONS_TOTAL; i++)
		for (j = 0; j<a->nb_vehic; j++)
			sem_post(a->sem_vehicule+j);
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

		/* parcours de la liste */
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

