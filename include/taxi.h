/** @file */
#ifndef _TAXI_H_
#define _TAXI_H_

/** crée les threads taxi et attend leur terminaison.
 * peut sortir avec EXIT_FAILURE en cas d'erreur
 * @param[in] nom_fifo nom du pipe nommé d'où les passagers seront lus
 */
void gerer_taxis(const char* nom_fifo);

/** simule un taxi.
 * reçoit un passager du pipe nommé, le conduit à sa destination
 * @param[in] arg arguments nécessaires pour un taxi, type struct arg_taxi *
 */
void *taxi(void *arg);

#endif
