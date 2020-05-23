/** @file */
#define _POSIX_SOURCE
#include "transport.h"
#include "taxi.h"
#include "liste.h"
#include "constantes.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>

#define MAX_PATH_LEN 256
#define MAX_STR_LEN 256

/** lit les passagers à partir d'un fichier et les met dans les stations.
 * La fonction utilise le fichier passé dans les paramètres pour créer les
 * passagers.
 * La première ligne du fichier doit contenir un seul nombre - le nombre de
 * passagers dans le fichier.
 * Chaque ligne consécutive commence par un dièse (#) et une espace.
 * Après la ligne doit contenir 6 entiers séparés par des espaces correspondant
 * dans l'ordre aux informations présentes dans la structure #passager_t.
 *
 * Le tableau de stations doit être de taille NB_STATIONS_TOTAL.
 * Chaque élément du tableau doit être une liste initialisée, possiblement vide.
 * @param[in] path chemin vers le fichier contenant les passagers
 * @param[in,out] stations tableau de stations initialisées
 */
void lire_passagers(const char *path, liste_t **stations);

/** lit les trajets à partir d'un fichier et les ajoute à la liste de trajets
 * La fonction utilise le fichier passé dans les paramètres pour définir les
 * trajets des véhicules (bus et métro).
 * La première ligne du fichier doit contenir deux nombres : le nombre de bus
 * et le nombre de métros.
 *
 * Chaque ligne consécutive définit l'itinéraire d'un véhicule. On énumère
 * d'abord les trajets des bus et ensuite ceux des métros.
 * Une contient quelques nombres qui ne se répètent pas dans une même ligne.
 * Ces nombres représentent les stations desservies par le véhicule.
 *
 * On suppose qu'un bus parcours ses stations circulairement et un métro fait
 * des allers-retours.
 * @param[in] path chemin vers le fichier contenant les trajets
 * @param[in,out] trajets liste de trajets
 */
int lire_trajets(const char *path, liste_t *trajets);

/** la fonction principale du programme.
 * Le programme attend 2 paramètres : chemin vers le fichier avec les passagers
 * et chemin vers le fichier avec les trajets.
 *
 * Cette fonction lit les passagers et les trajets, crée un processus fils.
 * Le processus fils gère les taxi à travers la fonction #gerer_taxis().
 * Le processus père gère les transport en commun à travers #gerer_transport().
 *
 * À la fin de l'exécution la fonction affiche le revenu total.
 */
int main(int argc, char **argv) {
	liste_t **stations;
	liste_t *trajets;
	char path_pass[MAX_PATH_LEN];
	char path_trajet[MAX_PATH_LEN];
	pid_t pid;
	int i, err, nb_bus;
	long somme;
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
		gerer_taxis(nom_fifo);
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

		somme = gerer_transport(nom_fifo, stations, trajets, nb_bus);

		free(stations);
		/* envoie du signal de fin au processus fils et attente de sa terminaison */
		kill(pid, SIGUSR1);
		if (waitpid(pid, &err, 0) == -1) {
			perror("waitpid");
			return EXIT_FAILURE;
		}
		if (WEXITSTATUS(err) == EXIT_FAILURE)
			return EXIT_FAILURE;

		printf("Somme : %ld\n", somme);
	}
	return EXIT_SUCCESS;
}

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

int lire_trajets(const char *path, liste_t *liste_trajets) {
	int i, nb_bus, nb_metro;
	union int_pvoid tmp;
	char str[MAX_STR_LEN];
	char *token;
	struct maillon *m;
	FILE *f;
	liste_t *trajet;

	f = fopen(path, "r");
	if (f == NULL) {
		perror("fopen du fichier trajets");
		exit(EXIT_FAILURE);
	}

	fscanf(f, "%d %d\n", &nb_bus, &nb_metro);
	for (i = 0; i<nb_bus+nb_metro; i++) {
		trajet = liste_init();
		fscanf(f,"%[^\n]", str);
		token = strtok(str, " ");
		while (token != NULL) {
			tmp.i = atoi(token);
			liste_add_queue(trajet, tmp.p);
			token = strtok(NULL, " ");
		}
		fscanf(f,"\n");

		if (i >=nb_bus) {
			/* simulation des aller-retours pour un metro
			** (0,1,2,3) -> (2,1,0,1,2,3)*/
			if (trajet->taille > 0) {
				for (m = trajet->tete->suivant; m && m->suivant; m = m->suivant)
					liste_add_tete(trajet, m->donnee);
			}
		}

		liste_add_queue(liste_trajets, trajet);
	}
	fclose(f);
	return nb_bus;
}
