#ifndef GLOBALCLOCK_H
#define GLOBALCLOCK_H
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

typedef struct globalClock
{
	int globalClockCellInfo[NUM_OF_CELLS][2]; //holds infection level and population influx for each cell for a month.
	int totalMsgsFromCellsThisMonth;	
	int numOfAliveSquirrels ;		
	int numofInfectedSquirrels ;
	int currentMonth;
}globalClock;

globalClock 	gc;
void printOutput();
void initGlobalClock();
void sendChangeMonthCmd();
int globalClockCode(simulationMsg** queue,int queueSize,int* actorIds);


#endif