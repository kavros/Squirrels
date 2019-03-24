
#include "cell.h"
#include "squirrel.h"
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

