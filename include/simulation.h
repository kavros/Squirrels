#ifndef SIMULATION_H
#define SIMULATION_H
#include <stdbool.h>

#define TOTAL_MONTHS  24
#define NUM_OF_CELLS  16
extern int 	 MAX_NUM_OF_SQUIRRELS;
extern int 	 NUM_OF_SQUIRRELS ;
extern int 	 INITIAL_NUM_OF_INFECTED_SQUIRRELS ;
extern bool  isActorInitialized ;
extern long  state;


typedef enum simulationMsgCommands
{
	UPDATE_MONTH = 0,
	TERMINATE_ACTOR =1,
	UPDATE_INFECTED_SQUIRRELS=2,
	UPDATE_DEAD_SQUIRRELS=3,
	UPDATE_ALIVE_SQUIRRELS=4
}simulationMsgCommands;

typedef enum actorType
{
	SQUIRREL=0,	
	GLOBAL_CLOCK=2,
	CELL=3,
}actorType;

typedef struct simulationMsg
{
	int sqState;    
	int actorType;
	int populationInflux;
	int infectionLevel;
	float x;  
	float y; 
	simulationMsgCommands command;
}simulationMsg;

/**
* Returns the actor id based on the cell number.
**/
int getActorIdFromCell(int cellNum);

/**
* Returns the cell number based on actor id.
**/
int getCellNumFromActorId(int actorId);

/**
* Returns the actor id of the global clock.
**/
int getGlobalClockActorId();

/**
* Read the command line arguments tand initialize the variables appropriate.
* This function use an external library call argTable3.
* Notice: We did not check if the user enters invalid values such as negative numbers.
**/
int initCmdLineArgs(int argc, char *argv[],int actorId);


#endif