#ifndef _CONSTANTES_H_
#define _CONSTANTES_H_

#define NB_STATIONS_BUS 5
#define NB_STATIONS_METRO 3
#define NB_STATIONS_TOTAL (NB_STATIONS_BUS+2*NB_STATIONS_METRO)
#define CAPACITE_BUS 5
#define CAPACITE_METRO 8

#define LEN(A) sizeof(A)/sizeof(A[0])
#define STATION_DIR_DECR(A) (A)<NB_STATIONS_BUS ? (A) : (2*(A)-NB_STATIONS_BUS)
#define STATION_DIR_CROI(A) (A)<NB_STATIONS_BUS ? (A) : (2*(A)-NB_STATIONS_BUS+1)
#define NUM_STATION(A) (A)<NB_STATIONS_BUS ? (A) : (((A)+NB_STATIONS_BUS)/2)

static const int correspondance[] = {5, -1, -1, -1, -1, 0, -1, -1};

union int_pvoid {
	void *p;
	int i;
};

typedef struct {
	long id;
	int station_depart;
	int station_arrivee;
	int temps_ecoule;
	int transfert;
	int temps_att_max;
} passager_t;

#endif
