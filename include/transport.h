#ifndef _TRANSPORT_H_
#define _TRANSPORT_H_

#include "liste.h"
#include <pthread.h>

long creer_transport(const char*, liste_t**, liste_t*, int);
void *vehicule(void*);
int debarquer(liste_t*, liste_t **, int, int, pthread_mutex_t *);
int embarquer(liste_t*, liste_t **, int, int, pthread_mutex_t *);
void *verificateur(void*);
int verifier_passagers(liste_t**, int, long*);

#endif
