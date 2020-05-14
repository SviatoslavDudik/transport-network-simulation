#include "liste.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

liste_t *liste_init() {
	liste_t *l;
	l = (liste_t*) malloc(sizeof(liste_t));
	if (l == NULL) {
		fprintf(stderr, "Erreur allocation liste\n");
		exit(EXIT_FAILURE);
	}
	l->tete = NULL;
	l->queue = NULL;
	l->taille = 0;
	return l;
}

void liste_detruire(liste_t *l) {
	assert(l!=NULL);
	while (l->taille > 0)
		liste_rem_tete(l);
	free(l);
}

struct maillon *new_maillon(void *donnee) {
	struct maillon *m;
	m = (struct maillon*) malloc(sizeof(struct maillon));
	if (m == NULL) {
		fprintf(stderr, "Erreur allocation maillon\n");
		exit(EXIT_FAILURE);
	}
	m->donnee = donnee;
	m->suivant = NULL;
	return m;
}

void liste_add_tete(liste_t *l, void *donnee) {
	struct maillon *m;
	assert(l!=NULL);
	m = new_maillon(donnee);
	m->suivant = l->tete;
	l->tete = m;
	if (l->taille == 0)
		l->queue = m;
	l->taille += 1;
}

void liste_add_queue(liste_t *l, void *donnee) {
	struct maillon *m;
	assert(l!=NULL);
	m = new_maillon(donnee);
	if (l->taille == 0)
		l->tete = m;
	else
		l->queue->suivant = m;
	l->queue = m;
	l->taille += 1;
}

void *liste_rem_tete(liste_t *l) {
	struct maillon *m;
	void *d;
	assert(l!=NULL);
	assert(l->taille>0);
	m = l->tete;
	d = m->donnee;
	l->tete = l->tete->suivant;
	free(m);
	l->taille -= 1;

	if (l->taille == 0)
		l->queue = NULL;
	return d;
}

void *liste_rem_suivant(liste_t *l, struct maillon *prec) {
	struct maillon *m;
	void *d;
	assert(l!=NULL);
	assert(l->taille>0);
	assert(prec != l->queue);
	if (prec == NULL)
		return liste_rem_tete(l);

	m = prec->suivant;
	prec->suivant = m->suivant;
	if (l->queue == m)
		l->queue = prec;

	d = m->donnee;
	free(m);
	l->taille -= 1;
	return d;
}

int liste_vide(liste_t *l) {
	return l->taille == 0;
}

