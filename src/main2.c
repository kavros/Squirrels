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

typedef struct squirrel
{
	bool isInfected;
	float x;
	float y;
	int cell;
	
	int totalPopulationInflux; 		// totalpopulationInflux for the last 50 steps
	int totalInfectionLevel;		// totalInfectionLevel for the last 50 steps
}squirrel;

typedef struct cell
{
	int populationInflux;			
	double infectionLevel;	
}cell;


simulationMsg startData;
int cellNumStart;
long state;


int getActorIdFromCell(int cellNum)
{
	return cellNumStart+cellNum+1;
}




void squirrelDoComputeWork(simulationMsg** msgQueue,int* msgsInQueueCnt)
{

}



void squirrelCode()
{
	printf("SQUIRREL\n");
	/*squirrel sq;
	sq.isInfected 				= startData.isInfected;
	sq.x 		  				= startData.x;
	sq.y 		  				= startData.y;
	sq.totalPopulationInflux    = 0;
	sq.totalInfectionLevel 		= 0;
	int totalSteps				= 0;
	simulationMsg msgQueue[AC_MAX_MSG_QUEUE_SIZE];
	int 		  actorIdQueue[AC_MAX_MSG_QUEUE_SIZE];
	int msgsInQueueCnt			= 0;
	int currentMonth			= 0;



	squirrelStep(sq.x,sq.y,&(sq.x),&(sq.y),&state);
	sq.cell = getCellFromPosition(sq.x,sq.y);
	
	int cellActorId = getActorIdFromCell(sq.cell);
	//printf("squirrel step to  cell %d with actorId %d \n",nextCell,actorId);
	simulationMsg msg;
	msg.actorType = SQUIRREL;
	msg.isInfected = sq.isInfected;
	
	do
	{
		int outstanding =0;
		AC_Iprobe(&outstanding);
		if(!outstanding)
		{
			
			//handle messages in queue
			for(int i=0; i < msgsInQueueCnt; i++)
			{
				if(msgQueue[i].actorType == GLOBAL_CLOCK)
				{
					currentMonth++;
					//printf("sq %d month %d\n", AC_GetActorId(),currentMonth);
					simulationMsg msgToGC;
					msgToGC.actorType = SQUIRREL;
					msgToGC.isInfected = sq.isInfected;

					AC_Bsend(&msgToGC,actorIdQueue[i]);
				}
				else if(msgQueue[i].actorType == CELL)
				{

				}
				else
				{
					assert(0);
				}
			}
			msgsInQueueCnt = 0;


			//do a step
			totalSteps++;
		}
		else
		{
			int sourceActorId = AC_Recv(&msg);
			memcpy(&(msgQueue[msgsInQueueCnt]),&msg,sizeof(msg));
			actorIdQueue[msgsInQueueCnt] = sourceActorId;
			msgsInQueueCnt++;
		}
		
	}while(currentMonth != TOTAL_MONTHS);
*/

	//AC_Recv(&msg);
	

}

void cellCode()
{
	printf("CELL\n");

	/*cell cl;
	cl.populationInflux 	= 0;
	cl.infectionLevel 	= 0;

	//printf("cell %d\n",AC_GetActorId());
	simulationMsg msg;
	//int srcActorId = AC_Recv(&msg);
	//printf("actor id %d received from %d\n",AC_GetActorId(),srcActorId );
	AC_Recv(&msg);
	//printf("%d\n",msg.actorType );*/
}

void globalClockCode()
{	
	printf("GLOBAL_CLOCK\n");
	/*printf("global clock\n");
	simulationMsg sendMsg;
	simulationMsg recvMsg;
	printf("-----%d\n",recvMsg.isInfected );
	sendMsg.actorType = GLOBAL_CLOCK;
	for(int i=0; i < TOTAL_MONTHS; i++)
	{
		AC_Bcast(&sendMsg,AC_GetActorId());

		AC_Recv(&recvMsg);
		printf("%d %d\n",recvMsg.actorType,recvMsg.isInfected );
		sleep(1);
	}*/
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