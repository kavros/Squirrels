#ifndef SQUIRREL_H
#define SQUIRREL_H
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



typedef enum squirrelState
{
	SQUIRREL_IS_INFECTED = 1,
	SQUIRREL_IS_HEALTHY = 4
}squirrelState;


typedef struct squirrel
{
	enum squirrelState sqState;
	float x;
	float y;
	int cell;
	int stepsCnt;
	int populationInflux[50]; 		
	int infectionLevel[50];	
	int infectedSteps;	
}squirrel;

squirrel sqData;


void initSquirrelData(squirrelState st);
void resetSquirrelData();
int squirrelCode(simulationMsg** queue,int queueSize,int* actorIds);

#endif