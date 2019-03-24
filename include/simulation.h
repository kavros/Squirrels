#ifndef SIMULATION_H
#define SIMULATION_H
#include <stdbool.h>
#define TOTAL_MONTHS  24
#define NUM_OF_CELLS  16
extern int MAX_NUM_OF_SQUIRRELS;
extern int NUM_OF_SQUIRRELS ;
extern int INITIAL_NUM_OF_INFECTED_SQUIRRELS ;
extern bool 			isActorInitialized ;
extern long 			state;


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


int getActorIdFromCell(int cellNum);
int getCellNumFromActorId(int actorId);
int getGlobalClockActorId();
int initCmdLineArgs(int argc, char *argv[],int actorId);


#endif