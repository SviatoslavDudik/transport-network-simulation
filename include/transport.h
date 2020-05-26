/** @file */
#ifndef _TRANSPORT_H_
#define _TRANSPORT_H_

#include "liste.h"
#include <pthread.h>
#include <semaphore.h>

/** Gère le système de transport publique.
 * La fonction crée les véhicules et le vérificateur et attend leur terminaison.
 *
 * stations est un tableau de listes de passagers.
 * Chaque station est une liste de passagers représentant une file.
 * La taille du tableau est supérieur au nombre de stations.
 * En effet, les stations de métro ont deux sens, on a une file par sens.
 * Les terminus ont aussi deux sens mais seulement un est utilisé.
 * On suppose que dans le tableau il y a d'abord #NB_STATIONS_BUS stations de
 * bus suivis de 2*#NB_STATIONS_METRO files de métro.
 * Le sens est déterminé avec l'ordre des stations: ordre croissant de numéros
 * ou ordre décroissant (comme dans un arbre de recherche).
 *
 * La liste de trajets doit être de taille égale au nombre total de véhicules.
 * La liste doit contenir les itinéraires des bus suivis des ceux des métros.
 * Chaque élément est, donc, une liste de stations qui composent le trajet.
 * @param[in] nom_fifo nom du fichier correspondant au pipe nommé
 * @param[in] stations tableau de toutes les stations du système
 * @param[in] trajets liste de trajets des véhicules
 * @param[in] nb_bus nombre de bus
 * @return la somme payée par les passagers
 */
long gerer_transport(const char *nom_fifo, liste_t **stations, liste_t *trajets, int nb_bus);

/** Crée les threads associés aux transports.
 * La fonction crée les threads des véhicules en leur passant les arguments
 * nécessaires.
 *
 * Les 3 premiers paramètres correspondent aux arguments de la fonction
 * #gerer_transport().
 *
 * Les 2 paramètres suivants sont des tableau de sémaphores de taille égale au
 * nombre de véhicules. Les sémaphores doivent être crées et initialisés avec
 * la valeur 0 (et on peut laisser pshared nul).
 * Ces paramètres doivent être les même que ceux passés à la fonction
 * #creer_verificateur().
 * @param[in] stations tableau de toutes les stations du système
 * @param[in] trajets liste des trajets
 * @param[in] nb_bus nombre de bus
 * @param[in] sem_vehicule tableau de sémaphores pour débloquer les véhicules
 * @param[in] sem_verif tableau de sémaphores pour débloquer le vérificateur
 * @param[in] sem_nonvides sémaphore indiquant le nombre de véhicules non vides
 * @param[in] mutex mutex permettant d'accéder aux stations avec une
 * correspondance, doit être initialisés auparavant
 * @param[in] termine pointeur sur une variable booléenne valant vrai si le
 * programme est terminé
 * @return tableau de threads créés
 * @see gerer_transport() pour les détails de stations
 */
pthread_t *creer_transport(liste_t **stations, liste_t *trajets, int nb_bus, sem_t *sem_vehicule, sem_t *sem_verif, sem_t *sem_nonvides, pthread_mutex_t *mutex, int *termine);

/** Crée le threads du vérificateur.
 * La fonction crée le vérificateur en lui passant les arguments nécessaires.
 *
 * Les tableaux de sémaphores doivent être de taille égale au nombre de
 * véhicules. Les sémaphores doivent être crées et initialisés avec la valeur 0
 * (et on peut laisser pshared nul).
 * Ces paramètres doivent être les même que ceux passés à la fonction
 * #creer_transport().
 * @param[in] stations tableau de toutes les stations du système
 * @param[in] sem_vehicule tableau de sémaphores pour débloquer les véhicules
 * @param[in] sem_verif tableau de sémaphores pour débloquer le vérificateur
 * @param[in] sem_nonvides sémaphore indiquant le nombre de véhicules non vides
 * @param[in] fifo descripteur de fichier du pipe nommé
 * @param[in] nb_vehicules nombre total de véhicules
 * @param[in] termine pointeur sur une variable booléenne valant vrai si le
 * programme est terminé
 * @return pointeur vers le thread créé
 * @see gerer_transport() pour les détails de stations
 */
pthread_t *creer_verificateur(liste_t **stations, sem_t *sem_vehicule, sem_t *sem_verif, sem_t *sem_nonvides, int fifo, int nb_vehicules, int *termine);

/** Attend la terminaison de tous les threads et renvoie le revenu.
 * Après execution de cette fonction tous les threads associés aux transports et au vérificateur sont terminés.
 *
 * Les deux premiers paramètres correspondent respectivement aux valeurs de retour des fonctions #creer_transport() et #creer_verificateur().
 * @param[in] pthread_vehicule les threads des véhicules
 * @param[in] pthread_verif le thread de vérificateur
 * @param[in] taille taille du tableau pthread_vehicule, donc égale au nombre
 * total des véhicules
 * @return somme payée par les passagers, transports en commun et taxis compris
 */
long calculer_revenu(pthread_t *pthread_vehicule, pthread_t *pthread_verif, int taille);

/** Simule un véhicule (bus ou métro) en circulation.
 * Embarque et débarque les passagers en comptant le revenu.
 *
 * La capacité du véhicule se détermine avec son type:
 * #CAPACITE_BUS pour un bus,
 * #CAPACITE_METRO pour un métro.
 *
 * Un bus commence à la première station de la liste trajet.
 * La station initiale d'un métro  se trouve à la moitié de la liste trajet.
 * Plus précisément, la station initiale pour un métro est d'indice
 * taille_du_trajet/2 - 1 (les indices à partir de 0).
 *
 * Les cycles sont synchronisés avec le vérificateur à l'aide d'un rendez-vous
 * bilatéral. Pour cela les sémaphores de la structure sont utilisés.
 *
 * La valeur de retour est de type long* et peut être récupérer avec
 * pthread_join.
 * Le pointeur renvoyé doit être free.
 * @param[in] arg pointeur vers une structure #arg_vehicule allouée sur le tas
 * (heap). Il sera free dans la fonction.
 * @return le revenu
 */
void *vehicule(void *arg);

/** Embarque les passagers dans un véhicule.
 *
 * stations[indice_st] donne directement la file d'attente nécessaire.
 *
 * La fonction tiens compte de la capacité du véhicule.
 *
 * @param[in,out] passagers liste des passagers du véhicule
 * @param[in,out] stations tableau de toutes les stations
 * @param[in] indice_st indice de la station dans le tableau de stations (ne
 * correspond pas forcément à la numération conventionnelle de stations)
 * @param[in] metro valeur booléenne indiquant si le véhicule est un métro
 * @param[in] mutex_corresp mutex permettant d'accéder aux stations avec une
 * correspondance
 * @return nombre de passagers embarqués
 * @see gerer_transport() pour les détails de stations
 */
int embarquer(liste_t *passagers, liste_t **stations, int indice_st, int metro, pthread_mutex_t *mutex_corresp);

/** Débarque du véhicule les passagers qui sont arrivés à la destination.
 * Si un passager a besoin du transfert et la station permet une
 * correspondance, alors la fonction transfèrera le passager à la station
 * correspondante.
 *
 * Si un passager est arrivé à sa destination il sera effacer et n'apparaitra
 * dans aucune file.
 * @param[in,out] passagers liste des passagers du véhicule
 * @param[in,out] stations tableau de toutes les stations
 * @param[in] num_station numéro de la station (correspond aux numéros de
 * stations utilisé dans la structure #passager_t)
 * @param[in] metro valeur booléenne indiquant si le véhicule est un métro
 * @param[in] mutex_corresp mutex permettant d'accéder aux stations avec une
 * correspondance
 * @return nombre de passagers débarqués
 * @see gerer_transport() pour les détails de stations
 */
int debarquer(liste_t *passagers, liste_t **stations, int num_station, int metro, pthread_mutex_t *mutex_corresp);

/** Vérifie le temps d'attente des passagers, les transfert vers les taxis.
 * Cette fonction se synchronise avec les véhicule et exécute la vérification à
 * la fin de chaque cycle.
 * Elle envoie les passagers qui souhaitent prendre un taxi dans le pipe nommé.
 *
 * Elle examine également s'il reste encore des passagers. Quand tous les
 * passagers sont arrivés à leurs destinations, elle met #arg_verif.termine à
 * vrai ce qui entraine la terminaison des threads-véhicules.
 *
 * La valeur de retour est de type long* et peut être récupérer avec
 * pthread_join.
 * Le pointeur renvoyé doit être free.
 * @param[in] arg pointeur vers une structure #arg_verif allouée sur le tas
 * (heap). Il sera free dans la fonction.
 * @return le revenu provenant du taxi
 */
void *verificateur(void *arg);

/** Incrémente le temps d'attente des passagers et les envoie vers les taxis.
 * La fonction incrémente le temps d'attente des passagers dans toutes les
 * stations.
 *
 * Elle envoie un passager leur dans le pipe nommé quand son temps d'attente
 * atteint son temps d'attente maximal.
 *
 * La fonction compte le revenu du taxi.
 * @param[in,out] stations tableau de toutes les stations
 * @param[in] fifo descripteur de fichier du pipe nommé.
 * @param[in,out] sum montant du revenu des taxis
 * @return valeur booléenne valant vrai si toutes les stations sont vides
 * @see gerer_transport() pour les détails de stations
 */
int verifier_passagers(liste_t **stations, int fifo, long *sum);

#endif
