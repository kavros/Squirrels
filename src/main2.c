#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stddef.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "squirrel-functions.h"
#include "actor.h"
#include "squirrel.h"
#include "cell.h"
#include "globalClock.h"

int MAX_NUM_OF_SQUIRRELS = 200;
int NUM_OF_SQUIRRELS = 34;
int INITIAL_NUM_OF_INFECTED_SQUIRRELS = 4;
bool 			isActorInitialized = false;
long 			state;


int main(int argc, char *argv[])
{

	AC_Init(argc,argv);
	
	int actorId = AC_GetActorId();
	int val = initCmdLineArgs(argc,argv,actorId);
	if(val == 1)
	{

		AC_Finalize();
		return -1;
	}

	//printf("my rank = %d \n",actorId);
	long seed = -1-actorId;
	state =seed;
	initialiseRNG(&seed);
	//AC_setMaxNumberOfActors();
	
	int numOfActors 			= NUM_OF_SQUIRRELS+NUM_OF_CELLS+1;
	int actorTypes 				= 3;
	int (*func_ptrs[3])() 		= {squirrelCode,cellCode,globalClockCode};
	int actorTypes_quantity[3] 	= {NUM_OF_SQUIRRELS,NUM_OF_CELLS,1};
	AC_SetActorTypes( numOfActors,actorTypes, actorTypes_quantity,func_ptrs);

	int msgFields 							= 7;
	AC_Datatype msgDataTypeForEachField[7] 	= {AC_INT,AC_INT,AC_INT,AC_INT,AC_FLOAT,AC_FLOAT,AC_INT};
	int blockLen[7]							= {1,1,1,1,1,1,1};
	//printf("sizeof(struct) = %d\n",sizeof(simulationMsg));
	MPI_Aint simulationMsgDisp[7];
	simulationMsgDisp[0] = offsetof(simulationMsg, sqState);
	simulationMsgDisp[1] = offsetof(simulationMsg, actorType);
	simulationMsgDisp[2] = offsetof(simulationMsg, populationInflux);
	simulationMsgDisp[3] = offsetof(simulationMsg, infectionLevel);
	simulationMsgDisp[4] = offsetof(simulationMsg, x);
	simulationMsgDisp[5] = offsetof(simulationMsg, y);
	simulationMsgDisp[6] = offsetof(simulationMsg, command);
	


	AC_SetActorMsgDataType(msgFields, msgDataTypeForEachField,blockLen,simulationMsgDisp);


	if(actorId >=1 && actorId <= NUM_OF_SQUIRRELS)
	{
 		//startData.actorType = SQUIRREL;
 		if(actorId <= INITIAL_NUM_OF_INFECTED_SQUIRRELS)
 		{
 			
 			 initSquirrelData( SQUIRREL_IS_INFECTED);
 		}
 		else
 		{
 			initSquirrelData( SQUIRREL_IS_HEALTHY);
 		}
 		//printf("s---------%d\n",AC_GetActorId());
 	}
 	else if(actorId >= (NUM_OF_SQUIRRELS+1) && actorId <= (NUM_OF_SQUIRRELS+NUM_OF_CELLS))
 	{
 		
 		initCellData();
 		//printf("c---------%d\n",AC_GetActorId());
 	}
 	/*else if(actorId == NUM_OF_SQUIRRELS+NUM_OF_CELLS+1)
 	{

 		initGlobalClock();
 		//printf("cl---------%d\n",AC_GetActorId());
 	}*/
 	
 	
	
	if(actorId > NUM_OF_SQUIRRELS+NUM_OF_CELLS+1)
	{
		initSquirrelData( SQUIRREL_IS_HEALTHY);
		isActorInitialized = false;
	}

	

	AC_RunSimulation();
	

	AC_Finalize();
	return 0;

}