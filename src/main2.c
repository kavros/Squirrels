#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stddef.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "squirrel-functions.h"
#include "pool.h"
#include "actor.h"


#define NUM_OF_CELLS  16
#define MAX_NUM_OF_SQUIRRELS  200
#define NUM_OF_SQUIRRELS 34
#define TOTAL_MONTHS 24



typedef enum actorType
{
	SQUIRREL=1,	
	GLOBAL_CLOCK=2,
	CELL=3,
}actorType;

typedef struct simulationMsg
{
	int command;
	int actorType;
	int populationInflux;
	int infectionLevel;
	float x;
	float y;
	bool isInfected;
}simulationMsg;

simulationMsg startData;
int cellNumStart;
long state;


int getActorIdFromCell(int cellNum)
{
	return cellNumStart+cellNum+1;
}





void squirrelCode()
{
	squirrelStep(startData.x,startData.y,&(startData.x),&(startData.y),&state);
	int nextCell = getCellFromPosition(startData.x,startData.y);
	
	int actorId = getActorIdFromCell(nextCell);
	//printf("squirrel step to  cell %d with actorId %d \n",nextCell,actorId);

}

void cellCode()
{
	printf("cell %d\n",AC_GetActorId());

}

void globalClockCode()
{	
	printf("global clock\n");

}


int main(int argc, char *argv[])
{

	AC_Init(argc,argv);
	int actorId = AC_GetActorId();
	//printf("my rank = %d \n",actorId);
	long seed = -1-actorId;
	state =seed;
	initialiseRNG(&seed);
	//AC_setMaxNumberOfActors();
	

	int numOfActors 			= NUM_OF_SQUIRRELS+NUM_OF_CELLS+1;
	int actorTypes 				= 3;
	void (*func_ptrs[3])() 		= {squirrelCode,cellCode,globalClockCode};
	int actorTypes_quantity[3] 	= {NUM_OF_SQUIRRELS,NUM_OF_CELLS,1};
	AC_SetActorTypes( numOfActors,actorTypes, actorTypes_quantity,func_ptrs);

	int msgFields 							= 7;
	AC_Datatype msgDataTypeForEachField[7] 	= {AC_INT,AC_INT,AC_INT,AC_INT,AC_FLOAT,AC_FLOAT,AC_BOOL};
	int blockLen[7]							= {1,1,1,1,1,1,1};
	MPI_Aint simulationMsgDisp[7];
	simulationMsgDisp[0] = offsetof(simulationMsg, command);
	simulationMsgDisp[1] = offsetof(simulationMsg, actorType);
	simulationMsgDisp[2] = offsetof(simulationMsg, populationInflux);
	simulationMsgDisp[3] = offsetof(simulationMsg, infectionLevel);
	simulationMsgDisp[4] = offsetof(simulationMsg, x);
	simulationMsgDisp[5] = offsetof(simulationMsg, y);
	simulationMsgDisp[7] = offsetof(simulationMsg, isInfected);

	AC_SetActorMsgDataType(msgFields, msgDataTypeForEachField,blockLen,simulationMsgDisp);
	

	startData.x =0;
	startData.y=0;
	if(actorId >=1 && actorId < NUM_OF_SQUIRRELS)
	{
		
 		startData.actorType = SQUIRREL;
 		if(actorId <= 4)
 		{
 			
 			startData.isInfected = true;
 		}
 		else
 		{
 			startData.isInfected = false;
 		}

 	}
 	else if(actorId >= NUM_OF_SQUIRRELS && actorId < (NUM_OF_SQUIRRELS+NUM_OF_CELLS))
 	{
 		
 		startData.actorType = CELL;


 	}
 	else if(actorId >= NUM_OF_SQUIRRELS+NUM_OF_CELLS && actorId < (NUM_OF_SQUIRRELS+NUM_OF_CELLS+1))
 	{

 		startData.actorType = GLOBAL_CLOCK;

 	}
 	

	cellNumStart = NUM_OF_SQUIRRELS;

	AC_RunSimulation();
	

	AC_Finalize();
	return 0;

}