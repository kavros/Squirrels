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

/**
* Cells and squirrels are starting	without waiting for the global clock so we need an extra position
* inside arrays for keeping the initial population influx and infection level.
**/
typedef struct cell
{
	int populationInflux[TOTAL_MONTHS+1];				
	int infectionLevel[TOTAL_MONTHS+1];		
	int currentMonth;
}cell;

// Every cell use this global variable in order the state.
cell 			cellData;

/**
* The framework calls this function for all cell actors.
* There are more comments in cell.c.
**/
int cellCode(simulationMsg** msgQueue,int queueSize,int* actorIdsQueue);

/**
* Initialize the members of the global struct variable cellData.
**/
void initCellData();


#endif
