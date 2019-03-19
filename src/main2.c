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
#define INITIAL_NUM_OF_INFECTED_SQUIRRELS 4



typedef enum actorType
{
	SQUIRREL=0,	
	GLOBAL_CLOCK=2,
	CELL=3,
}actorType;

typedef enum squirrelState
{
	SQUIRREL_IS_INFECTED = 1,
	SQUIRREL_IS_HEALTHY = 4
}squirrelState;

typedef enum simulationMsgCommands
{
	UPDATE_MONTH = 0,
	TERMINATE_ACTOR =1,
	UPDATE_INFECTED_SQUIRRELS=2,
	UPDATE_DEAD_SQUIRRELS=3,
	UPDATE_ALIVE_SQUIRRELS=4
}simulationMsgCommands;

typedef struct simulationMsg
{
	squirrelState sqState;    
	int actorType;
	int populationInflux;
	int infectionLevel;
	float x;  
	float y; 
	simulationMsgCommands command;
}simulationMsg;

typedef struct squirrel
{
	squirrelState sqState;
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
	int populationInflux[TOTAL_MONTHS+1];			
	int infectionLevel[TOTAL_MONTHS+1];		// cells and squirrels are starting	without waiting for the global clock so we need an extra
											// for keeping the initial population influx and infection level
	int currentMonth;
}cell;


typedef struct globalClock
{
	int globalClockCellInfo[NUM_OF_CELLS][2]; //holds infection level and population influx for each cell for a month.
	int totalMsgsFromCellsThisMonth;	
	int numOfAliveSquirrels ;		
	int numofInfectedSquirrels ;
	int currentMonth;
}globalClock;




long 			state;
bool 			isActorInitialized = false;
squirrel sqData;
cell cellData;
globalClock gc;


int getGlobalClockActorId()
{
	return NUM_OF_SQUIRRELS+NUM_OF_CELLS+1;
}


int getActorIdFromCell(int cellNum)
{
	int cellNumStart = NUM_OF_SQUIRRELS+1;
	return cellNumStart+cellNum;
}

int getCellNumFromActorId(int actorId)
{
	return actorId - NUM_OF_SQUIRRELS -1;
}




void initSquirrelData(squirrelState st)
{
	sqData.sqState 					= st;
	sqData.x						= 0;
	sqData.y 						= 0;
	sqData.stepsCnt  				= 0;
	sqData.infectedSteps			= 0;
	for(int i=0; i<50; i++){ sqData.infectionLevel[i] =0; sqData.populationInflux[i] =0;}
	simulationMsg recvMsg;
	//AC_Recv()

}
void resetSquirrelData()
{
	initSquirrelData(SQUIRREL_IS_HEALTHY);
	isActorInitialized		= false;
	
}
void waitStartSignal()
{
	simulationMsg recvMsg;
	AC_Recv(&recvMsg);
	assert(recvMsg.command == UPDATE_MONTH);
	assert(recvMsg.actorType == GLOBAL_CLOCK);
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
void initCellData()
{
	isActorInitialized =true;
	for(int i=0; i < TOTAL_MONTHS; i++)
	{
		cellData.populationInflux[i] 	= 0;
		cellData.infectionLevel[i]		= 0;
	}
	cellData.currentMonth = 0;
}


int  getPopulationInfluxForLast3Months()
{
	int populationInflux = 0;
	if(cellData.currentMonth >= 2)
	{
		populationInflux = 
			cellData.populationInflux[cellData.currentMonth] +
			cellData.populationInflux[cellData.currentMonth-1] +
			cellData.populationInflux[cellData.currentMonth -2];
	}
	else if(cellData.currentMonth == 1)
	{
		populationInflux = 
			cellData.populationInflux[cellData.currentMonth] +
			cellData.populationInflux[cellData.currentMonth-1];
	}
	else if( cellData.currentMonth == 0)
	{
		populationInflux = 
			cellData.populationInflux[cellData.currentMonth];	
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
	if(cellData.currentMonth == 0)
	{
		infectionLevel = cellData.infectionLevel[cellData.currentMonth]; 
	}
	else
	{
		infectionLevel = 
			cellData.infectionLevel[cellData.currentMonth-1]+
			cellData.infectionLevel[cellData.currentMonth];
	}
	
	return infectionLevel;
}

int cellCode(simulationMsg** msgQueue,int queueSize,int* actorIdsQueue)
{
	//printf("CELL\n" );

	
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
				if(recvMsg->command == TERMINATE_ACTOR){
					
					return TERMINATE_ACTOR;
				}
				else if(recvMsg->command == UPDATE_MONTH)
				{
					//printf("[CELL %d] received update month command\n",AC_GetActorId() );
					sendMsg.actorType = CELL;
					sendMsg.populationInflux = getPopulationInfluxForLast3Months();
					sendMsg.infectionLevel   = getInfectionLevelForLast2Months();
					destActorId =actorIdsQueue[i]; 
					//if(currentMonth == 4) printf("%d\n",cellData.infectionLevel[currentMonth-2] );
					AC_Bsend(&sendMsg,destActorId);	
					//printf("cell %d send msg to GC p:%d i:%d \n",getCellNumFromActorId(AC_GetActorId()),sendMsg.populationInflux,sendMsg.infectionLevel);
					cellData.currentMonth = cellData.currentMonth+1;
					assert(cellData.currentMonth>=0 && cellData.currentMonth <= TOTAL_MONTHS);
				}
				else
				{
					assert(0);
				}
				break;
			case SQUIRREL:
				
				cellData.populationInflux[cellData.currentMonth] = cellData.populationInflux[cellData.currentMonth]+1;
				if(recvMsg->sqState == SQUIRREL_IS_INFECTED)
				{
					cellData.infectionLevel[cellData.currentMonth]   = cellData.infectionLevel[cellData.currentMonth]+1;
				}
				

				sendMsg.actorType = CELL;
				sendMsg.populationInflux = getPopulationInfluxForLast3Months();
				sendMsg.infectionLevel = getInfectionLevelForLast2Months();
				//if(sendMsg.infectionLevel != 0)printf("------%d\n",sendMsg.infectionLevel);

				destActorId =actorIdsQueue[i];
				//if(recvMsg->isInfected )printf("cell %d received SQUIRREL %d \n",AC_GetActorId(),destActorId);
				//printf("---- %d \n",recvMsg->command);
				AC_Bsend(&sendMsg,destActorId);
				break;

		}
	
	}
	/*if(cellData.currentMonth == TOTAL_MONTHS-1)
		printf("[CELL %d] will terminate\n",AC_GetActorId());*/

	return AC_KEEP_ACTOR_ALIVE;

	//return isTerminate();
	
}




void printOutput()
{

	printf("[Global Clock] Month %d has %d alive and %d infected squirrels\n",gc.currentMonth,gc.numOfAliveSquirrels,gc.numofInfectedSquirrels);
	printf("[Global Clock] \t Num of cell \t  PopulationInflux \t InfectionLevel\n");
	for(int i=0; i <NUM_OF_CELLS; i++)
	{
		printf("\t \t %d \t\t %d \t\t\t %d\n",i,gc.globalClockCellInfo[i][0],gc.globalClockCellInfo[i][1]);
	}
	printf("\n");
}

void initGlobalClock()
{
	for(int i=0;i<NUM_OF_CELLS;i++)
	{
		gc.globalClockCellInfo[i][0] = 0;
		gc.globalClockCellInfo[i][1] = 0;
	}
	
	gc.totalMsgsFromCellsThisMonth = 0;
	gc.numOfAliveSquirrels = NUM_OF_SQUIRRELS;
	gc.numofInfectedSquirrels = INITIAL_NUM_OF_INFECTED_SQUIRRELS;
	gc.currentMonth = 0;
	isActorInitialized=true;
}


void sendChangeMonthCmd()
{
	
	//if(cellMsgs < 16)
	simulationMsg sendMsg;
	sendMsg.actorType = GLOBAL_CLOCK;
	sendMsg.command = UPDATE_MONTH;
	//AC_Bcast(&sendMsg,AC_GetActorId());
	
	for(int cellNum=0; cellNum < 16; cellNum++)
	{
		int actorId = getActorIdFromCell(cellNum);
		
		AC_Bsend(&sendMsg,actorId);
	}
	usleep(50000);
	gc.currentMonth =gc.currentMonth +1;

}

int globalClockCode(simulationMsg** queue,int queueSize,int* actorIds)
{	
	if(isActorInitialized == false)
	{
		initGlobalClock();
		sendChangeMonthCmd();
	}
	simulationMsg* recvMsg;
	int cellNum;
	//printf("----- %d\n",AC_GetActorId() );
	for(int i=0; i < queueSize; i++)
	{
		recvMsg = queue[i];
		switch(recvMsg->actorType)
		{
			case CELL:
				//printf("GLOBAL_CLOCK received message form cell %d !\n",getCellNumFromActorId(actorIds[i]));
				//printf("%d %d",getCellNumFromActorId(actorIds[i]),recvMsg->infectionLevel,recvMsg->populationInflux);
				cellNum = getCellNumFromActorId(actorIds[i]);
				gc.globalClockCellInfo[cellNum][0]	= recvMsg->populationInflux;
				gc.globalClockCellInfo[cellNum][1]	= recvMsg->infectionLevel;
				gc.totalMsgsFromCellsThisMonth 		= gc.totalMsgsFromCellsThisMonth+1;
				break;
			case GLOBAL_CLOCK:
				assert(0);			
				break;
			case SQUIRREL:
				if(recvMsg->command == UPDATE_DEAD_SQUIRRELS)
				{
					//printf("Received msg from dead squirrel %d \n",actorIds[i]);
					gc.numOfAliveSquirrels = gc.numOfAliveSquirrels-1;
					gc.numofInfectedSquirrels = gc.numofInfectedSquirrels-1;
				}
				else if(recvMsg->command == UPDATE_INFECTED_SQUIRRELS)
				{
					//printf("Received msg from infected squirrel %d \n",actorIds[i]);
					gc.numofInfectedSquirrels=gc.numofInfectedSquirrels+1;
				}
				else if(recvMsg->command == UPDATE_ALIVE_SQUIRRELS)
				{
					gc.numOfAliveSquirrels=gc.numOfAliveSquirrels+1;
					if(gc.numOfAliveSquirrels == MAX_NUM_OF_SQUIRRELS){
						
						printf("[GLOBAL_CLOCK ERROR] number of squirrels is %d which exceeds the limit of %d \n",gc.numOfAliveSquirrels,MAX_NUM_OF_SQUIRRELS);
						simulationMsg lastGCMsg;
						lastGCMsg.actorType = GLOBAL_CLOCK;
						lastGCMsg.command = TERMINATE_ACTOR;
						AC_Bcast(&lastGCMsg,AC_GetActorId());
						return AC_TERMINATE_ACTOR;
					}
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



	bool haveMsgsFromCellsArrived	 = (gc.totalMsgsFromCellsThisMonth == (NUM_OF_CELLS));
	bool isThisTheLastMonth 		 = (gc.currentMonth == TOTAL_MONTHS );
	if( ! haveMsgsFromCellsArrived )
	{
		return AC_KEEP_ACTOR_ALIVE;
	}
	gc.totalMsgsFromCellsThisMonth=0;


	
	if(isThisTheLastMonth)
	{

		simulationMsg lastGCMsg;
		lastGCMsg.actorType = GLOBAL_CLOCK;
		lastGCMsg.command = TERMINATE_ACTOR;
		AC_Bcast(&lastGCMsg,AC_GetActorId());
		printOutput();	

		return AC_TERMINATE_ACTOR;
	}
	else
	{
		printOutput();
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