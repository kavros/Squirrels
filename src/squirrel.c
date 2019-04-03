#include "squirrel.h"

/**
* Every squirrel call this function at the start and initialize the data.
**/
void initSquirrelData(squirrelState st)
{
	sqData.sqState 					= st;
	sqData.x						= 0;
	sqData.y 						= 0;
	sqData.stepsCnt  				= 0;
	sqData.infectedSteps			= 0;
	for(int i=0; i<50; i++){ sqData.infectionLevel[i] =0; sqData.populationInflux[i] =0;}
}

/**
* Squirrels resets their data when they die because it 
* is possible to use the same actor when a squirrel give a birth.
**/
void resetSquirrelData()
{
	initSquirrelData(SQUIRREL_IS_HEALTHY);
}

/**
* A squirrel does a step and sleeps for a fixed ammount of time.
**/
void squirrelDoesAStep()
{
	simulationMsg sendMsg;
	sendMsg.actorType 	= SQUIRREL;
	sendMsg.sqState 	= sqData.sqState;

	squirrelStep(sqData.x,sqData.y,&(sqData.x),&(sqData.y),&state);
	sqData.cell 	= getCellFromPosition(sqData.x,sqData.y);
	int cellActorId = getActorIdFromCell(sqData.cell);
	
	assert(cellActorId >= (NUM_OF_SQUIRRELS+1) && cellActorId <= (NUM_OF_SQUIRRELS+NUM_OF_CELLS));
//	printf("[SQUIRREL %d] does a step to %d\n",AC_GetActorId(), cellActorId );
	AC_Bsend(&sendMsg,cellActorId);
	/**
	* Squirrel sleeps because we observed that if we dont 
	* add it then they die at the very start of the simulation.
	**/
	usleep(1000);
	sqData.stepsCnt =sqData.stepsCnt+1;
}

/**
* Squirrels run this function every 50 steps in order to decide if 
* they will give a birth. Every time that a squirrel reproduce use framework to 
* create a new actor of type squirrel with the specified start data.
* In addtion, the parent informs the global clock that the squirrel is giving birth.
**/
void willSquirrelReproduce()
{
	int sumPopulationInflux=0;
	float avgPopulationInflux=0;	
	
	for(int i=0; i < 50;i++) sumPopulationInflux += sqData.populationInflux[i];
	avgPopulationInflux = ((float)sumPopulationInflux)/50.0f;
	bool willReproduce = willGiveBirth(avgPopulationInflux,&state);
	if(willReproduce == true)
	{
		
		
		simulationMsg childStartData;
		childStartData.x = sqData.x;
		childStartData.y = sqData.y;

		AC_CreateNewActor(SQUIRREL,&childStartData);
//		printf("[Squirrel %d] will reproduce at (%f,%f) \n",AC_GetActorId(),sqData.x,sqData.y);

		simulationMsg newSquirrelBirth;
		newSquirrelBirth.actorType = SQUIRREL;
		newSquirrelBirth.command   = UPDATE_ALIVE_SQUIRRELS;
		AC_Bsend(&newSquirrelBirth, getGlobalClockActorId());
	}
	
}
/**
* Squirrels run this function every 50 steps in order to decide if 
* they will catch the dissease. Every time that a squirrel catch the dissease
* informs the global clock.
**/
void willSquirrelCatchDisease()
{
	int avgInfectionLevel =0,sumInfectionLevel=0;
	if(sqData.sqState == SQUIRREL_IS_HEALTHY)
	{
		for(int i=0; i < 50;i++)sumInfectionLevel +=sqData.infectionLevel[i];
		avgInfectionLevel = ((float)sumInfectionLevel)/50.0f;
		
		//printf("avg infection level is %f\n",avgInfectionLevel );
		if(willCatchDisease(avgInfectionLevel,&state) == 1)
		{ 
			//printf("[SQUIRREL %d] avginfection level is %f\n",AC_GetActorId(),avgInfectionLevel );
			sqData.sqState =SQUIRREL_IS_INFECTED;
			simulationMsg sendMsg;
			sendMsg.actorType = SQUIRREL;
			sendMsg.command   = UPDATE_INFECTED_SQUIRRELS;
			//printf("[Squirrel %d] is infected\n",AC_GetActorId());
			AC_Bsend(&sendMsg,getGlobalClockActorId());
		}		
	}
}

/**
* After every 50 infected steps the squirrels check if it will die.
* Before the squirrel die informs the global clock and reset the squirrel's data.
**/
int  willSquirrelDie()
{
	if(sqData.sqState == SQUIRREL_IS_INFECTED)
	{
		sqData.infectedSteps++;
		if(sqData.infectedSteps >= 50)
		{
			int die = willDie(&state);
			if(die == true)
			{

				//printf("[Squirrel %d] died !\n",AC_GetActorId());
				simulationMsg sendMsg;
				sendMsg.actorType = SQUIRREL;
				sendMsg.command = UPDATE_DEAD_SQUIRRELS;				
				AC_Bsend(&sendMsg,getGlobalClockActorId());
				resetSquirrelData();
				return AC_TERMINATE_ACTOR;
			}
		}
		
	}
	return AC_KEEP_ACTOR_ALIVE;
}

int squirrelCode(simulationMsg** queue,int queueSize,int* actorIds)
{
	simulationMsg* recvMsg;
	
	for(int i=0; i < queueSize; i++)
	{
		recvMsg = queue[i];
		switch(recvMsg->actorType)
		{
			case GLOBAL_CLOCK:
//				printf("[SQUIRREL %d]  received termination msg from global clock\n",AC_GetActorId());
				
				// All squirrels terminates when they receive a termination message from global clock.
				if(recvMsg->command == TERMINATE_ACTOR)
				{
					
					return AC_TERMINATE_ACTOR;	
				}
				else
				{
					assert(0);
				}
				
				break;
			case CELL:
				// Get response from cell that did a step.
				sqData.populationInflux[sqData.stepsCnt] = sqData.populationInflux[sqData.stepsCnt] + recvMsg->populationInflux;
				sqData.infectionLevel[sqData.stepsCnt]   = sqData.infectionLevel[sqData.stepsCnt]+recvMsg->infectionLevel;
				assert(sqData.stepsCnt < 50);
				break;
			case SQUIRREL:
				/**
				* When a new squirrel is born, it retrives the initial position 
				* from the data that are already inside the queue.
				**/
				//printf("%d\n",recvMsg->actorType );
				assert(AC_GetParentActorId() != 0);	
				sqData.x = recvMsg->x;
				sqData.y = recvMsg->y;
//				printf("[Squirrel %d] was born at position (%f,%f) !\n",AC_GetActorId(),sqData.x,sqData.y);
				break;

		}
	}
	

	// Squirrel does a random step.
	squirrelDoesAStep();

	if( willSquirrelDie() == AC_TERMINATE_ACTOR)
	{
		return AC_TERMINATE_ACTOR;
	}

	// Every 50 steps, we chech if the squirrel will give a birth or catch the disease.
	if(sqData.stepsCnt == 49)
	{
		willSquirrelReproduce();

		willSquirrelCatchDisease();		

		// Resets value to zero every 50 steps because we use it for indexing struct arrays.
		sqData.stepsCnt = 0;
	}
	return AC_KEEP_ACTOR_ALIVE;
}

