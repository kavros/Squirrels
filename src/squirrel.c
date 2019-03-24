#include "squirrel.h"

void initSquirrelData(squirrelState st)
{
	sqData.sqState 					= st;
	sqData.x						= 0;
	sqData.y 						= 0;
	sqData.stepsCnt  				= 0;
	sqData.infectedSteps			= 0;
	for(int i=0; i<50; i++){ sqData.infectionLevel[i] =0; sqData.populationInflux[i] =0;}
	simulationMsg recvMsg;
}

void resetSquirrelData()
{
	initSquirrelData(SQUIRREL_IS_HEALTHY);
	isActorInitialized		= false;
}


int squirrelCode(simulationMsg** queue,int queueSize,int* actorIds)
{
	//printf("SQUIRREL\n");
	
	simulationMsg* recvMsg;
	if(isActorInitialized == false)
	{
		//initSquirrelData();

		if(AC_GetParentActorId() != 0) // if squirrel born from another squirel instead of master
		{
			bool isParentMsgInsideQueue = false;
			for(int i=0;i < queueSize; i++){
				if(actorIds[i] == AC_GetParentActorId())
				{
					recvMsg = queue[i];				
					if(recvMsg->actorType == SQUIRREL )
					{
						//printf("parent msg is inside the queue dont wait\n" );
						sqData.x=recvMsg->x;
						sqData.y=recvMsg->y;
						isParentMsgInsideQueue = true;

					}
				}
			}
			
			if(isParentMsgInsideQueue == false)
			{
				simulationMsg parentData;
				AC_GetStartData(&parentData);
				sqData.x=parentData.x;
				sqData.y=parentData.y;
			}

//			printf("[Squirrel %d] was born at position (%f,%f) !\n",AC_GetActorId(),sqData.x,sqData.y);
			assert(sqData.sqState == SQUIRREL_IS_HEALTHY );
		}
		
		isActorInitialized = true;

		
		assert(sqData.infectedSteps == 0);
		assert(sqData.stepsCnt == 0);
		assert(isActorInitialized == true);
	}

	for(int i=0; i < queueSize; i++)
	{
		recvMsg = queue[i];
		switch(recvMsg->actorType)
		{
			case GLOBAL_CLOCK:
				//printf("[SQUIRREL %d]  received termination msg from global clock\n",AC_GetActorId());
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
				sqData.populationInflux[sqData.stepsCnt] = sqData.populationInflux[sqData.stepsCnt] + recvMsg->populationInflux;
				//if(recvMsg->infectionLevel != 0)printf("%d\n",recvMsg->infectionLevel);
				sqData.infectionLevel[sqData.stepsCnt]   = sqData.infectionLevel[sqData.stepsCnt]+recvMsg->infectionLevel;
				assert(sqData.stepsCnt <= 50);
				break;

		}
	}
	


	// squirrel does a random step
	simulationMsg sendMsg;
	sendMsg.actorType 	= SQUIRREL;
	sendMsg.sqState 	= sqData.sqState;

	squirrelStep(sqData.x,sqData.y,&(sqData.x),&(sqData.y),&state);
	sqData.cell 	= getCellFromPosition(sqData.x,sqData.y);
	int cellActorId = getActorIdFromCell(sqData.cell);
	AC_Bsend(&sendMsg,cellActorId);
	usleep(1000);

	sqData.stepsCnt =sqData.stepsCnt+1;

	if(sqData.sqState == SQUIRREL_IS_INFECTED)
	{
		sqData.infectedSteps++;
		if(sqData.infectedSteps >= 50)
		{
			int die = willDie(&state);
			if(die == true)
			{

				//printf("[Squirrel %d] died !\n",AC_GetActorId());
				sendMsg.actorType = SQUIRREL;
				sendMsg.command = UPDATE_DEAD_SQUIRRELS;
				//printf("%d\n",globalClockActorId );
				AC_Bsend(&sendMsg,getGlobalClockActorId());
				resetSquirrelData();
				return AC_TERMINATE_ACTOR;
			}
		}
		
	}

	// decide about state (born,infected)
	if(sqData.stepsCnt == 50)
	{
		//printf("---squirrel---\n");
		int sumPopulationInflux=0,sumInfectionLevel = 0;
		float avgPopulationInflux=0,avgInfectionLevel=0;

		
		
		for(int i=0; i < 50;i++) sumPopulationInflux += sqData.populationInflux[i];
		avgPopulationInflux = ((float)sumPopulationInflux)/50.0f;
		bool willReproduce = willGiveBirth(avgPopulationInflux,&state);
		if(willReproduce == true)
		{
			
			
			simulationMsg childStartData;
			childStartData.x = sqData.x;
			childStartData.y = sqData.y;
			AC_CreateNewActor(SQUIRREL,&childStartData);
			//printf("[Squirrel %d] will reproduce at (%f,%f) \n",AC_GetActorId(),sqData.x,sqData.y);

			simulationMsg newSquirrelBirth;
			newSquirrelBirth.actorType = SQUIRREL;
			newSquirrelBirth.command   = UPDATE_ALIVE_SQUIRRELS;
			AC_Bsend(&newSquirrelBirth, getGlobalClockActorId());
		}
		

		if(sqData.sqState == SQUIRREL_IS_HEALTHY)
		{
			for(int i=0; i < 50;i++)sumInfectionLevel +=sqData.infectionLevel[i];
			avgInfectionLevel = ((float)sumInfectionLevel)/50.0f;
			
			//printf("avg infection level is %f\n",avgInfectionLevel );
			if(willCatchDisease(avgInfectionLevel,&state) == 1)
			{ 
				//printf("[SQUIRREL %d] avginfection level is %f\n",AC_GetActorId(),avgInfectionLevel );
				sqData.sqState =SQUIRREL_IS_INFECTED;
				
				sendMsg.actorType = SQUIRREL;
				sendMsg.command   = UPDATE_INFECTED_SQUIRRELS;
				//printf("[Squirrel %d] is infected\n",AC_GetActorId());
				AC_Bsend(&sendMsg,getGlobalClockActorId());
			}
			
		}

		sqData.stepsCnt = 0;
	}
	return AC_KEEP_ACTOR_ALIVE;
	//return isTerminate();
}
