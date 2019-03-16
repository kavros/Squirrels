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
#define INITIAL_NUM_OF_INFECTED_SQUIRRELS 4;



typedef enum actorType
{
	SQUIRREL=1,	
	GLOBAL_CLOCK=2,
	CELL=3,
}actorType;

typedef enum commandTypes
{
	SQUIRREL_IS_DEAD = 14,
	SQUIRREL_IS_INFECTED = 1,
	SQUIRREL_IS_BORN =3
}commandTypes;

typedef struct simulationMsg
{
	int isInfected;
	int command;    // ?
	int actorType;
	int populationInflux;
	int infectionLevel;
	float x;  // ?
	float y;  // ?

}simulationMsg;

typedef struct squirrel
{
	int isInfected;
	float x;
	float y;
	int cell;
	int stepsCnt;
	int populationInflux[50]; 		
	int infectionLevel[50];	
	int infectedSteps;	
}squirrel;

typedef struct cell
{
	int populationInflux[TOTAL_MONTHS];			
	int infectionLevel[TOTAL_MONTHS];	
}cell;


simulationMsg 	startData;
int 			cellNumStart;
long 			state;
int 			currentMonth = 0;
bool 			isActorInitialized = false;
int 			globalClockActorId;

squirrel sqData;
cell cellData;

int getActorIdFromCell(int cellNum)
{
	return cellNumStart+cellNum;
}

int getCellNumFromActorId(int actorId)
{
	return actorId - NUM_OF_SQUIRRELS -1;
}

bool isTerminate()
{
	if(currentMonth == (TOTAL_MONTHS-1)){ //because we start from 0 month
		// printf("actor %d  reach end of month\n",AC_GetActorId() );
		return AC_TERMINATE_ACTOR;
	}
	return AC_KEEP_ACTOR_ALIVE;
}


void initSquirrelData()
{
	sqData.isInfected 				= startData.isInfected;
	sqData.x						= startData.x;
	sqData.y 						= startData.y;
	sqData.stepsCnt  				= 0;
	isActorInitialized 				= true;
	sqData.infectedSteps					= 0;
	for(int i=0; i<50; i++){ sqData.infectionLevel[i] =0; sqData.populationInflux[i] =0;}
}

int squirrelCode(simulationMsg** queue,int queueSize,int* actorIds)
{
	//printf("SQUIRREL\n");
	
	simulationMsg* recvMsg;
	if(isActorInitialized == false)
	{
		initSquirrelData();
	}

	for(int i=0; i < queueSize; i++)
	{
		recvMsg = queue[i];
		switch(recvMsg->actorType)
		{
			case GLOBAL_CLOCK:
				//printf("actor %d month is %d \n",AC_GetActorId(),currentMonth);	
				currentMonth++;
				break;
			case CELL:
				sqData.populationInflux[sqData.stepsCnt] = sqData.populationInflux[sqData.stepsCnt] + recvMsg->populationInflux;
				sqData.infectionLevel[sqData.stepsCnt]   = sqData.infectionLevel[sqData.stepsCnt]+recvMsg->infectionLevel;
				//AC_Bsend()
				break;
		}
	}
	


	// squirrel does a random step
	simulationMsg sendMsg;
	sendMsg.actorType 	= SQUIRREL;
	sendMsg.isInfected 	= sqData.isInfected;	
	squirrelStep(sqData.x,sqData.y,&(sqData.x),&(sqData.y),&state);
	sqData.cell 	= getCellFromPosition(sqData.x,sqData.y);
	int cellActorId = getActorIdFromCell(sqData.cell);
	/*if(sqData.isInfected){
		printf("infected squirrel %d step to actorId %d \n",AC_GetActorId(),cellActorId);
	}*/

	
	AC_Bsend(&sendMsg,cellActorId);
	sqData.stepsCnt =sqData.stepsCnt+1;

	if(sqData.isInfected == true)
	{
		sqData.infectedSteps++;
		if(sqData.infectedSteps >= 50)
		{
			int die = willDie(&state);
			if(die == true)
			{
				printf("squirrel %d died !\n",AC_GetActorId());
				sendMsg.command = SQUIRREL_IS_DEAD;
				sendMsg.actorType = SQUIRREL;
				//printf("%d\n",globalClockActorId );
				AC_Bsend(&sendMsg,globalClockActorId);
				return AC_TERMINATE_ACTOR;
			}
		}
		
	}

	// decide about state (born,infected)
	if(sqData.stepsCnt == 50)
	{

		int sumPopulationInflux=0,sumInfectionLevel = 0;
		float avgPopulationInflux=0,avgInfectionLevel=0;

		
		
		for(int i=0; i < 50;i++) sumPopulationInflux += sqData.populationInflux[i];
		avgPopulationInflux = ((float)sumPopulationInflux)/50.0f;
		bool willReproduce = willGiveBirth(avgPopulationInflux,&state);
		if(willReproduce == true)
		{
			//printf("squirrel will reproduce \n");

		}
		

		if(sqData.isInfected == false)
		{
			for(int i=0; i < 50;i++)sumInfectionLevel +=sqData.infectionLevel[i];
			avgInfectionLevel =  ((float)sumInfectionLevel)/50.0f;
			sqData.isInfected  = willCatchDisease(avgInfectionLevel,&state);
			//if(sqData.isInfected )printf("squirrel %d is infected\n",AC_GetActorId());
		}

		sqData.stepsCnt = 0;
	}

	return isTerminate();
}
void initCellData()
{
	isActorInitialized =true;
	for(int i=0; i < 50; i++)
	{
		cellData.populationInflux[i] 	= 0;
		cellData.infectionLevel[i]		= 0;
	}
}


int  getPopulationInfluxForLast3Months()
{
	int populationInflux = 0;
	if(currentMonth >= 2)
	{
		populationInflux = 
			cellData.populationInflux[currentMonth] +
			cellData.populationInflux[currentMonth-1] +
			cellData.populationInflux[currentMonth -2];
	}
	else if(currentMonth == 1)
	{
		populationInflux = 
			cellData.populationInflux[currentMonth] +
			cellData.populationInflux[currentMonth-1];
	}
	else if( currentMonth == 0)
	{
		populationInflux = 
			cellData.populationInflux[currentMonth];	
	}
	else
	{
		assert(0);
	}
	return populationInflux;
}

int getInfectionLevelForLast2Months()
{
	int infectionLevel = 0;
	if(currentMonth == 0)
	{
		infectionLevel = cellData.infectionLevel[currentMonth]; 
	}
	else
	{
		infectionLevel = 
			cellData.infectionLevel[currentMonth-1]+
			cellData.infectionLevel[currentMonth];
	}
	//printf("--%d \n",infectionLevel);
	return infectionLevel;
}

int cellCode(simulationMsg** msgQueue,int queueSize,int* actorIdsQueue)
{
	//printf("CELL\n" );

	if(isActorInitialized == false)
	{
		initCellData();
	}
	simulationMsg sendMsg;
	int destActorId;
	simulationMsg* recvMsg;
	for(int i=0; i < queueSize; i++)
	{
		recvMsg = msgQueue[i];
		switch(recvMsg->actorType)
		{
			case GLOBAL_CLOCK:
				//printf("actor %d month is %d \n",AC_GetActorId(),currentMonth);
				sendMsg.actorType = CELL;
				sendMsg.populationInflux = getPopulationInfluxForLast3Months(cellData);
				sendMsg.infectionLevel   = getInfectionLevelForLast2Months(cellData);
				destActorId =actorIdsQueue[i]; 
				//if(currentMonth == 4) printf("%d\n",cellData.infectionLevel[currentMonth-2] );
				AC_Bsend(&sendMsg,destActorId);	
				//printf("cell %d send msg to GC p:%d i:%d \n",getCellNumFromActorId(AC_GetActorId()),sendMsg.populationInflux,sendMsg.infectionLevel);
				currentMonth++;
				break;
			case SQUIRREL:
				
				cellData.populationInflux[currentMonth] = cellData.populationInflux[currentMonth]+1;
				cellData.infectionLevel[currentMonth]   = cellData.infectionLevel[currentMonth]+recvMsg->isInfected;
				//if(recvMsg->isInfected ) printf("++%d \n",recvMsg->isInfected);

				sendMsg.actorType = CELL;
				sendMsg.populationInflux = getPopulationInfluxForLast3Months(cellData);
				sendMsg.populationInflux = getInfectionLevelForLast2Months(cellData);
				destActorId =actorIdsQueue[i];
				//if(recvMsg->isInfected )printf("cell %d received SQUIRREL %d \n",AC_GetActorId(),destActorId);
				//printf("---- %d \n",recvMsg->command);
				AC_Bsend(&sendMsg,destActorId);
				break;

		}
	
	}

	return isTerminate();
	
}
int globalClockCellInfo[NUM_OF_CELLS][2]; //holds infection level and population influx for each cell for a month.
int totalMsgsFromCellsThisMonth =0;
int numOfAliveSquirrels = NUM_OF_SQUIRRELS;
int numofInfectedSquirrels = INITIAL_NUM_OF_INFECTED_SQUIRRELS;


void sendChangeMonthCmd()
{
	
	//if(cellMsgs < 16)
	simulationMsg sendMsg;
	sendMsg.actorType = GLOBAL_CLOCK;
	AC_Bcast(&sendMsg,AC_GetActorId());
	usleep(500);
	currentMonth++;
	totalMsgsFromCellsThisMonth=0;

}

void printOutput()
{
	printf("month: %d\n",currentMonth);
	for(int i=0; i <NUM_OF_CELLS; i++)
	{
		printf("%d %d %d\n",i,globalClockCellInfo[i][0],globalClockCellInfo[i][1]);
	}
	printf("\n");
}

void initGlobalClock()
{
	for(int i=0;i<NUM_OF_CELLS;i++)
	{
		globalClockCellInfo[i][0] = 0;
		globalClockCellInfo[i][1] = 0;
	}
	isActorInitialized=true;
}

int globalClockCode(simulationMsg** queue,int queueSize,int* actorIds)
{	
	if(isActorInitialized == false)
	{
		initGlobalClock();
		sendChangeMonthCmd();
	}
		//printf("GLOBAL_CLOCK\n");
	simulationMsg* recvMsg;

	//printf("----- %d\n",AC_GetActorId() );
	for(int i=0; i < queueSize; i++)
	{
		recvMsg = queue[i];
		switch(recvMsg->actorType)
		{
			case CELL:
				//printf("GLOBAL_CLOCK received message form cell %d !\n",getCellNumFromActorId(actorIds[i]));
				//printf("%d %d",getCellNumFromActorId(actorIds[i]),recvMsg->infectionLevel,recvMsg->populationInflux);
				globalClockCellInfo[getCellNumFromActorId(actorIds[i])][0]=recvMsg->populationInflux;
				globalClockCellInfo[getCellNumFromActorId(actorIds[i])][1]=recvMsg->infectionLevel;
				totalMsgsFromCellsThisMonth++;
				//printf("Cell\n");
				break;
			case GLOBAL_CLOCK:
				assert(0);			
				break;
			case SQUIRREL:
				if(recvMsg->command == SQUIRREL_IS_DEAD)
				{
					printf("Received msg from dead squirrel %d \n",actorIds[i]);
				}
				else if(recvMsg->command == SQUIRREL_IS_INFECTED)
				{
					printf("Received msg from infected squirrel %d \n",actorIds[i]);
				}
				else if(recvMsg->command == SQUIRREL_IS_BORN)
				{
					
				}
				else
				{
					assert(0);
				}
				break;
			default:
				assert(0);
		}
	}



	bool haveMsgsFromCellsArrived	 = totalMsgsFromCellsThisMonth == (NUM_OF_CELLS);
	bool isThisTheLastMonth 		 = currentMonth == TOTAL_MONTHS -1;
	if( ! haveMsgsFromCellsArrived )
	{
		return AC_KEEP_ACTOR_ALIVE;
	}

	
	if(isThisTheLastMonth)
	{
		//printOutput();	
		return AC_TERMINATE_ACTOR;
	}
	else
	{
		//printOutput();
		sendChangeMonthCmd();	
	}
	
	

	
	return AC_KEEP_ACTOR_ALIVE ;
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
	int (*func_ptrs[3])() 		= {squirrelCode,cellCode,globalClockCode};
	int actorTypes_quantity[3] 	= {NUM_OF_SQUIRRELS,NUM_OF_CELLS,1};
	AC_SetActorTypes( numOfActors,actorTypes, actorTypes_quantity,func_ptrs);

	int msgFields 							= 7;
	AC_Datatype msgDataTypeForEachField[7] 	= {AC_INT,AC_INT,AC_INT,AC_INT,AC_FLOAT,AC_FLOAT,AC_INT};
	int blockLen[7]							= {1,1,1,1,1,1,1};
	//printf("sizeof(struct) = %d\n",sizeof(simulationMsg));
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
	if(actorId >=1 && actorId <= NUM_OF_SQUIRRELS)
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
 		//printf("s---------%d\n",AC_GetActorId());
 	}
 	else if(actorId >= (NUM_OF_SQUIRRELS+1) && actorId <= (NUM_OF_SQUIRRELS+NUM_OF_CELLS))
 	{
 		
 		startData.actorType = CELL;
 		//printf("c---------%d\n",AC_GetActorId());
 	}
 	else if(actorId == NUM_OF_SQUIRRELS+NUM_OF_CELLS+1)
 	{

 		startData.actorType = GLOBAL_CLOCK;
 		//printf("cl---------%d\n",AC_GetActorId());
 	}
 	
 	globalClockActorId = NUM_OF_SQUIRRELS+NUM_OF_CELLS+1;
	
	cellNumStart = NUM_OF_SQUIRRELS+1;

	AC_RunSimulation();
	

	AC_Finalize();
	return 0;

}