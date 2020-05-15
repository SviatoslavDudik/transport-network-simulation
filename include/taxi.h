/** @file */
#ifndef _TAXI_H_
#define _TAXI_H_

/** crée les threads taxi et attend leur terminaison.
 * peut sortir avec EXIT_FAILURE en cas d'erreur
 * @param[in] nom_fifo nom du pipe nommé d'où les passagers seront lus
 */
void creer_taxis(const char* nom_fifo);

/** handler pour un signal, entraine la terminaison des threads.
 * met #termine à 1 (vrai)
 * @param[in] sig numéro du signal
 */
void terminer(int sig);

/** simule un taxi.
 * reçoit un passager du pipe nommé, le conduit à sa destination
 * @param[in] arg arguments nécessaires pour un taxi, type struct arg_taxi *
 */
void *taxi(void *arg);

#endif
