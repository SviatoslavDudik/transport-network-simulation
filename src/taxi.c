#include "taxi.h"

void creer_taxis(const char *nom_fifo) {
	int fifo;
	fifo = open(nom_fifo, O_RDONLY);
	pause();
}
