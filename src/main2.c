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

/**
* We can change the 3 values below using 
* the appropriate flags when we run the programm.
* For more information run the program using -h flag.
**/
int MAX_NUM_OF_SQUIRRELS = 200;
int NUM_OF_SQUIRRELS = 34;
int INITIAL_NUM_OF_INFECTED_SQUIRRELS = 4;

bool 			isActorInitialized = false;
long 			state;

void initialiseSimulation()
{
	/*
	* Every actor has an actor id which use for many reasons (ex. initialization).
	* Notice that the framework uses the actorId=0 internal so
	* users need to initializate their actors starting from actor id equals to 1.
	*/
	int actorId = AC_GetActorId();

	// Initialize random number generator.
	long seed = -1-actorId;
	state =seed;
	initialiseRNG(&seed);

	/**
	* Initialize start data for each actor.
	**/
	if(actorId >=1 && actorId <= NUM_OF_SQUIRRELS)
	{
		// Initialize healthy and infected squirrels.
 		if(actorId <= INITIAL_NUM_OF_INFECTED_SQUIRRELS)
 		{
 			
 			initSquirrelData( SQUIRREL_IS_INFECTED);
 		}
 		else
 		{
 			initSquirrelData( SQUIRREL_IS_HEALTHY);
 		}
 	}
 	else if(actorId >= (NUM_OF_SQUIRRELS+1) && actorId <= (NUM_OF_SQUIRRELS+NUM_OF_CELLS))
 	{
 		// Initialize cells.
 		initCellData(); 		
 	}
 	
 	
	
	/**
	* The remaining actors are initialized as squirrels 
	* because we will possibly use the when a squirrel gives a birth.
	**/
	if(actorId > NUM_OF_SQUIRRELS+NUM_OF_CELLS+1)
	{
		initSquirrelData( SQUIRREL_IS_HEALTHY);
		isActorInitialized = false;
	}
}


int main(int argc, char *argv[])
{

	// Every actor need to call the initialization function of the actor pattern.
	AC_Init(argc,argv);
	
	/*
	* Every actor has an actor id which use for many reasons (ex. initialization).
	* Notice that the framework uses the actorId=0 internal so
	* users need to initializate their actors starting from actor id equals to 1.
	*/
	int actorId = AC_GetActorId();

	// Parsing command line arguments
	int val = initCmdLineArgs(argc,argv,actorId);

	// If the user run the programm using help flag we print a help message and terminate.
	if(val == 1)
	{
		AC_Finalize();
		return -1;
	}

	// Initialize squirrels, cells and global clock.
	initialiseSimulation();

	/**
	* Initialize and pass to the framework:
	* 	the total number of actors, 
	* 	the total number or different actors,
	* 	the funtions for each different type of actor.
	*   the quantity for each different type of actor.
	**/	
	int numOfActors 			= NUM_OF_SQUIRRELS+NUM_OF_CELLS+1;
	int actorTypes 				= 3;
	int (*func_ptrs[3])() 		= {squirrelCode,cellCode,globalClockCode};
	
	// The actor framework will use **actorTypes_quantity** array to determine how many actors of each type will wake up. 
	int actorTypes_quantity[3] 	= {NUM_OF_SQUIRRELS,NUM_OF_CELLS,1};
	AC_SetActorTypes( numOfActors,actorTypes, actorTypes_quantity,func_ptrs);


	/**
	* Describes the message that actors using for comunication.
	* The user need to pass to the framework:
	* 	the total fields of the message,
	* 	the type for each field
	*   the size of each type
	*   and the displacements
	**/
	int msgFields 							= 7;
	AC_Datatype msgDataTypeForEachField[7] 	= {AC_INT,AC_INT,AC_INT,AC_INT,AC_FLOAT,AC_FLOAT,AC_INT};
	int blockLen[7]							= {1,1,1,1,1,1,1};	
	AC_Aint simulationMsgDisp[7];
	simulationMsgDisp[0] = offsetof(simulationMsg, sqState);			// see file simulation.h which contains struct simulationMsg.
	simulationMsgDisp[1] = offsetof(simulationMsg, actorType);
	simulationMsgDisp[2] = offsetof(simulationMsg, populationInflux);
	simulationMsgDisp[3] = offsetof(simulationMsg, infectionLevel);
	simulationMsgDisp[4] = offsetof(simulationMsg, x);
	simulationMsgDisp[5] = offsetof(simulationMsg, y);
	simulationMsgDisp[6] = offsetof(simulationMsg, command);
	AC_SetActorMsgDataType(msgFields, msgDataTypeForEachField,blockLen,simulationMsgDisp);
	

	/**
	* Starts simulation.
	* It is important to mention here that actors start
	* without waiting for other actors.
	**/
	AC_RunSimulation();
	

	AC_Finalize();
	return 0;

}