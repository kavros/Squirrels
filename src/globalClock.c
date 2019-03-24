
#include "globalClock.h"
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