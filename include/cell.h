#ifndef CELL_H
#define CELL_H
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stddef.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "squirrel-functions.h"
#include "actor.h"
#include "simulation.h"


typedef struct cell
{
	int populationInflux[TOTAL_MONTHS+1];			
	int infectionLevel[TOTAL_MONTHS+1];		// cells and squirrels are starting	without waiting for the global clock so we need an extra
											// for keeping the initial population influx and infection level
	int currentMonth;
}cell;
cell 			cellData;

int cellCode(simulationMsg** msgQueue,int queueSize,int* actorIdsQueue);
int getInfectionLevelForLast2Months();
int  getPopulationInfluxForLast3Months();
void initCellData();


#endif
