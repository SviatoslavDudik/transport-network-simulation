#ifndef _LISTE_H_
#define _LISTE_H_

struct maillon {
	void *donnee;
	struct maillon *suivant;
};

typedef struct {
	struct maillon *tete;
	struct maillon *queue;
	unsigned long taille;
} liste_t;

liste_t *liste_init();
void liste_detruire(liste_t*);
struct maillon *new_maillon(void*);
void liste_add_tete(liste_t*, void*);
void liste_add_queue(liste_t*, void*);
void *liste_rem_tete(liste_t*);
void *liste_rem_suivant(liste_t*, struct maillon*);
int liste_vide(liste_t*);
#endif
