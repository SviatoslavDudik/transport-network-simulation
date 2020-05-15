#include "transport.h"
#include "taxi.h"
#include "liste.h"
#include "constantes.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#define MAX_PATH_LEN 256
#define MAX_STR_LEN 256

void lire_passagers(const char *path, liste_t **stations) {
	int i, n;
	FILE *f;
	passager_t *p;
	f = fopen(path, "r");
	if (f == NULL) {
		perror("fopen du fichier passagers");
		exit(EXIT_FAILURE);
	}
	fscanf(f,"%d\n", &n);
	for (i = 0; i<n; i++) {
		p = (passager_t*) malloc(sizeof(passager_t));
		if (p == NULL) {
			fprintf(stderr, "Erreur allocation passager\n");
			exit(EXIT_FAILURE);
		}
		fscanf(f, "# %ld %d %d %d %d %d\n",
				&(p->id), &(p->station_depart), &(p->station_arrivee),
				&(p->temps_ecoule), &(p->transfert), &(p->temps_att_max));
		if (p->station_depart < p->station_arrivee) {
			liste_add_queue(stations[STATION_DIR_CROI(p->station_depart)], p);
		} else {
			liste_add_queue(stations[STATION_DIR_DECR(p->station_depart)], p);
		}
	}
	fclose(f);
}

int lire_trajets(const char *path, liste_t *trajets) {
	int i, nb_bus, nb_metro;
	union int_pvoid tmp;
	char str[MAX_STR_LEN];
	char *token;
	struct maillon *m;
	FILE *f;
	liste_t *tr;
	f = fopen(path, "r");
	if (f == NULL) {
		perror("fopen du fichier trajets");
		exit(EXIT_FAILURE);
	}
	fscanf(f, "%d %d\n", &nb_bus, &nb_metro);
	for (i = 0; i<nb_bus+nb_metro; i++) {
		tr = liste_init();
		fscanf(f,"%[^\n]", str);
		token = strtok(str, " ");
		while (token != NULL) {
			tmp.i = atoi(token);
			liste_add_queue(tr, tmp.p);
			token = strtok(NULL, " ");
		}
		fscanf(f,"\n");

		if (i >=nb_bus) {
			/* simulation des aller-retours pour un metro
			** (0,1,2,3) -> (2,1,0,1,2,3)*/
			if (tr->taille > 0) {
				for (m = tr->tete->suivant; m && m->suivant; m = m->suivant)
					liste_add_tete(tr, m->donnee);
			}
		}

		liste_add_queue(trajets, tr);
	}
	fclose(f);
	return nb_bus;
}

int main(int argc, char **argv) {
	liste_t **stations;
	liste_t *trajets;
	char path_pass[MAX_PATH_LEN];
	char path_trajet[MAX_PATH_LEN];
	pid_t pid;
	int i, err, nb_bus;
	const char *nom_fifo = "fifo";
	if (argc == 3) {
		strncpy(path_pass, argv[1], MAX_PATH_LEN);
		strncpy(path_trajet, argv[2], MAX_PATH_LEN);

	} else {
		printf("Usage: %s <fichier_passagers> <fichier_trajets>", argv[0]);
		return EXIT_SUCCESS;
	}
	if (((err = mkfifo(nom_fifo, 0644)) != 0) && (err == EEXIST)) {
		perror("mkfifo");
		return EXIT_FAILURE;
	}

	pid = fork();
	if (pid == -1) {
		perror("Erreur fork taxi");
		return EXIT_FAILURE;
	}
	if (pid == 0) {
		creer_taxis(nom_fifo);
	} else {
		stations = malloc(sizeof(liste_t*)*NB_STATIONS_TOTAL);
		if (stations == NULL) {
			fprintf(stderr, "Erreur allocation stations\n");
			return EXIT_FAILURE;
		}
		for (i = 0; i<NB_STATIONS_TOTAL; i++) {
			stations[i] = liste_init();
		}
		trajets = liste_init();
		lire_passagers(path_pass, stations);
		nb_bus = lire_trajets(path_trajet, trajets);

		creer_transport(nom_fifo, stations, trajets, nb_bus);
		free(stations);
	}
	return EXIT_SUCCESS;
}
