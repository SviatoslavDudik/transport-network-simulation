/** @file */
#ifndef _CONSTANTES_H_
#define _CONSTANTES_H_

#define NB_STATIONS_BUS 5	/**< nombre de stations destinées aux bus */
#define NB_STATIONS_METRO 3	/**< nombre de stations destinées aux métro */
#define NB_STATIONS_TOTAL (NB_STATIONS_BUS+2*NB_STATIONS_METRO) /**< nombre total de stations */
#define CAPACITE_BUS 5		/**< capacité d'un bus */
#define CAPACITE_METRO 8	/**< capacité d'un métro */
#define NB_TAXIS 3			/**< nombre de taxis */

/** macro donnant l'indice de la file dans le sens décroissant.
 * @param[in] A le numéro de la station
 * @return l'indice de la file
 * @see #gerer_transport() pour les détailles sur les stations
 */
#define STATION_DIR_DECR(A) (A)<NB_STATIONS_BUS ? (A) : (2*(A)-NB_STATIONS_BUS)
/** macro donnant l'indice de la file dans le sens croissant.
 * @param[in] A le numéro de la station
 * @return l'indice de la file
 * @see #gerer_transport() pour les détailles sur les stations
 */
#define STATION_DIR_CROI(A) (A)<NB_STATIONS_BUS ? (A) : (2*(A)-NB_STATIONS_BUS+1)
/** macro donnant le numéro de la station à partir de son indice.
 * C'est un inverse des macros #STATION_DIR_CROI et #STATION_DIR_DECR.
 * @see #gerer_transport() pour les détailles sur les stations
 */
#define NUM_STATION(A) (A)<NB_STATIONS_BUS ? (A) : (((A)+NB_STATIONS_BUS)/2)

/** définie les correspondances entre des stations.
 * Si la station i n'a pas de correspondance, alors correspondance[i] = -1.
 *
 * Si la station i permet la correspondance à la station j, alors
 * correspondance[i] = j.
 */
static const int correspondance[] = {5, -1, -1, -1, -1, 0, -1, -1};

/** union permettant de stocker facilement des entier dans une liste */
union int_pvoid {
	void *p;
	int i;
};

/** structure représentant un passager */
typedef struct {
	long id;				/**< identifiant */
	int station_depart;		/**< numéro de la station de départ */
	int station_arrivee;	/**< numéro de la station d'arrivée */
	int temps_ecoule;		/**< temps que le passager a attendu */
	int transfert;			/**< booléen indiquant si le passager a besoin d'un transfert */
	int temps_att_max;		/**< temps d'attente maximal avant d'appeler le taxi */
} passager_t;

#endif
