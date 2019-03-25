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
	int populationInflux[50]; 		// Holds the populationInflux for the last 50 steps.
	int infectionLevel[50];			// Holds the infectionLevel for the last 50 steps.
	int infectedSteps;	
}squirrel;

// Evey squirrels use this value in order to keep state.
squirrel sqData;

/**
* Initialize values inside sqData global variable.
**/
void initSquirrelData(squirrelState st);

/**
* This function is passed to the framework. 
* Every squirrel use this function to implement the simulation logic.
**/
int squirrelCode(simulationMsg** queue,int queueSize,int* actorIds);

#endif