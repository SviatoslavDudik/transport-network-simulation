/** @file */
#ifndef _LISTE_H_
#define _LISTE_H_

/** structure de maillon d'une liste */
struct maillon {
	void *donnee;				/**< pointeur vers la donnée stockée */
	struct maillon *suivant;	/**< pointeur vers le maillon suivant */
};

/** structure de liste chainée */
typedef struct {
	struct maillon *tete;	/**< pointeur vers le premier maillon de la liste */
	struct maillon *queue;	/**< pointeur vers le dernier maillon de la liste */
	unsigned long taille;	/**< le nombre de maillons dans la liste */
} liste_t;

/** initialise et retourne une nouvelle liste.
 * La fonction alloue de la mémoire nécessaire.
 * La liste est initialement vide.
 * @return liste vide
 */
liste_t *liste_init();

/** libère la mémoire occupée par la liste.
 * La fonction supprime tous les maillons.
 * La liste doit être différente de NULL.
 * Attention, la fonction ne libère pas les données pointés par #maillon.donnee.
 * @param[in] l liste à libérer de la mémoire
 */
void liste_detruire(liste_t *l);

/** crée un nouveau maillon.
 * La fonction alloue de la mémoire et, donc, le maillon créé doit  être libérer avec free.
 * Le maillon créé contient la donnée passée en paramètre.
 * Le maillon a le champs suivant initialisé à NULL.
 * @param[in] donnee pointeur à stocker dans le maillon
 * @return maillon créé
 */
struct maillon *new_maillon(void *donnee);

/** ajoute un nouveau élément à la tête de la liste.
 * La fonction crée le maillon avec la donnée passée en paramètre. Ce maillon est placé au début de la liste.
 * La liste doit être différente de NULL.
 * @param[in] l liste où l'élément sera placé
 * @param[in] donnee la donnée qu'il faut ajouter au début de la liste
 */
void liste_add_tete(liste_t *l, void *donnee);

/** ajoute un nouveau élément à la fin de la liste.
 * La fonction crée le maillon avec la donnée passée en paramètre. Ce maillon est placé dans la queue de la liste.
 * @param[in] l liste où l'élément sera placé
 * @param[in] donnee la donnée qu'il faut ajouter à la fin de la liste
 */
void liste_add_queue(liste_t *l, void *donnee);

/** supprime le premier élément de la liste et renvoie sa valeur.
 * La fonction permet de récupérer le premier élément en le supprimant de la liste. Elle supprime le maillon associé de la mémoire.
 * La liste doit être différente de NULL et doit contenir au moins un élément.
 * @param[in] l liste non vide
 * @return la donnée du premier élément
 */
void *liste_rem_tete(liste_t *l);

/** supprime un élément de la liste et renvoie sa valeur.
 * La fonction permet de récupérer l'élément qui suit prec dans la liste. Elle supprime le maillon associé de la mémoire.
 *
 * La liste doit être différente de NULL et doit contenir au moins un élément.
 * Le maillon prec ne peut pas être dernier de la liste.
 * Pourtant il peut valoir NULL.
 * Dans ce cas, on supprime la tête de liste.
 * @param[in] l liste non vide
 * @param[in] prec l'élément précédant l'élément à supprimer
 * @return la donnée du premier élément
 */
void *liste_rem_suivant(liste_t *l, struct maillon *prec);
/** permet de savoir si la liste est vide.
 * La fonction renvoie vrai si la liste ne contient aucun élément.
 * Elle renvoie faux si la liste a au moins un élément.
 * @param[in] l liste différente de NULL
 * @return valeur booléenne indiquant si la liste est vide
 */
int liste_vide(liste_t *l);

#endif
