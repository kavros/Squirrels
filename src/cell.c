
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

/**
* Returns the population influx for the last 3 months.
**/
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

/**
* Returns the infection level for the last 2 months.
**/
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
	
	simulationMsg sendMsg;
	int destActorId;
	simulationMsg* recvMsg;
	for(int i=0; i < queueSize; i++)
	{
		recvMsg = msgQueue[i];
		switch(recvMsg->actorType)
		{
			/**
			* The cell receive two type of messages from the globalclock.
			* 	1) A termination message that instruct process to terminate immediatelly.
			*   2) A message that instruct cell to update the current month variable.	
			* In the second case the cell sends population influx and infection level values to global clock.
			**/
			case GLOBAL_CLOCK:
				if(recvMsg->command == TERMINATE_ACTOR){
					
					return TERMINATE_ACTOR;
				}
				else if(recvMsg->command == UPDATE_MONTH)
				{
					//printf("[CELL %d] received update month command\n",AC_GetActorId() );
					// Sends population influx and infection level values to global clock.
					sendMsg.actorType = CELL;
					sendMsg.populationInflux = getPopulationInfluxForLast3Months();
					sendMsg.infectionLevel   = getInfectionLevelForLast2Months();
					destActorId =actorIdsQueue[i]; 
					
					AC_Bsend(&sendMsg,destActorId);	
					cellData.currentMonth = cellData.currentMonth+1;
					assert(cellData.currentMonth>=0 && cellData.currentMonth <= TOTAL_MONTHS);
				}
				else
				{
					assert(0);
				}
				break;
			/**
			* When a cell receive a message from a squirrel
			* updates the populationInflux and infection level
			* and sends both values to squirrel.
			**/	
			case SQUIRREL:
				
				// Update cell.
				cellData.populationInflux[cellData.currentMonth] = cellData.populationInflux[cellData.currentMonth]+1;
				if(recvMsg->sqState == SQUIRREL_IS_INFECTED)
				{
					cellData.infectionLevel[cellData.currentMonth]   = cellData.infectionLevel[cellData.currentMonth]+1;
				}
				
				// Sends populationInflux and infection level values to squirrel.
				sendMsg.actorType = CELL;
				sendMsg.populationInflux = getPopulationInfluxForLast3Months();
				sendMsg.infectionLevel = getInfectionLevelForLast2Months();			
				destActorId =actorIdsQueue[i];
				AC_Bsend(&sendMsg,destActorId);
				break;

		}
	
	}
	

	return AC_KEEP_ACTOR_ALIVE;	
}

