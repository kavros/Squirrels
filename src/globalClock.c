#include "globalClock.h"

/**
* Print output to stdout every month.
**/
void printOutput()
{

	printf("[Global clock] Month %d has %d alive and %d infected squirrels\n",gc.currentMonth,gc.numOfAliveSquirrels,gc.numofInfectedSquirrels);
	printf("[Global Clock] \t Num of cell \t  PopulationInflux \t InfectionLevel\n");
	for(int i=0; i <NUM_OF_CELLS; i++)
	{
		printf("\t \t %d \t\t %d \t\t\t %d\n",i,gc.globalClockCellInfo[i][0],gc.globalClockCellInfo[i][1]);
	}
	printf("\n");
}

/**
* Initialize variables inside stuct globalClock.
**/
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

/**
* Sends a message to every that says that the months change.
* Then the clock sleep for defined milliseconds and updates the current month.
**/
void sendChangeMonthCmd()
{
	// Creates a message that asks from cells  to change month.
	simulationMsg sendMsg;
	sendMsg.actorType = GLOBAL_CLOCK;
	sendMsg.command = UPDATE_MONTH;
	
	// Sends message to every actor of type cell.
	for(int cellNum=0; cellNum < 16; cellNum++)
	{
		int actorId = getActorIdFromCell(cellNum);
		
		AC_Bsend(&sendMsg,actorId);
	}
	// Update global clock actor month.
	gc.currentMonth =gc.currentMonth +1;

	// Actor sleeps before move to the next month.
	usleep(MONTH_DURATION_IN_MILLISECONDS);
}

int globalClockCode(simulationMsg** queue,int queueSize,int* actorIds)
{	
	/** 
	* At start we initialize the global clock 
	* and we also send a message for changing month to every cell.
	**/
	if(isActorInitialized == false)
	{
		initGlobalClock();
		sendChangeMonthCmd();
	}


	simulationMsg* recvMsg;
	int cellNum;

	/**
	* Loop through the queue and respond to every message
	* based on the actor type of the sender.
	**/
	for(int i=0; i < queueSize; i++)
	{
		recvMsg = queue[i];
		switch(recvMsg->actorType)
		{
			/**
			*  We receive a response from a cell after we send an message for changing the month.
			*  We store the populationInflux and the infection level of each cell in order to 
			*  print them to stdout.
			**/
			case CELL:
				cellNum = getCellNumFromActorId(actorIds[i]);
				gc.globalClockCellInfo[cellNum][0]	= recvMsg->populationInflux;
				gc.globalClockCellInfo[cellNum][1]	= recvMsg->infectionLevel;
				gc.totalMsgsFromCellsThisMonth 		= gc.totalMsgsFromCellsThisMonth+1;
				break;
			// We dont expect receving a message from global clock.
			case GLOBAL_CLOCK:
				assert(0);			
				break;

			/**
			* Global clock receive a message from a squirrel in the following cases:
			* 	a squirrel dies
			*	a squirrel catch the disease
			*   a squirrel is giving a birth
			**/
			case SQUIRREL:
				if(recvMsg->command == UPDATE_DEAD_SQUIRRELS)
				{			
					gc.numOfAliveSquirrels = gc.numOfAliveSquirrels-1;
					gc.numofInfectedSquirrels = gc.numofInfectedSquirrels-1;
				}
				else if(recvMsg->command == UPDATE_INFECTED_SQUIRRELS)
				{
					gc.numofInfectedSquirrels=gc.numofInfectedSquirrels+1;
				}
				else if(recvMsg->command == UPDATE_ALIVE_SQUIRRELS)
				{
					gc.numOfAliveSquirrels=gc.numOfAliveSquirrels+1;

					/**
					* In case that the alive squirrels have exceed the limit
					* we send a message to all actors that instruct them to terminate and
					* the global clock terminates.
					**/
					if(gc.numOfAliveSquirrels == MAX_NUM_OF_SQUIRRELS){
						
						printf("[Global clock ERROR] number of squirrels is %d which exceeds the limit of %d \n",gc.numOfAliveSquirrels,MAX_NUM_OF_SQUIRRELS);
						simulationMsg lastGCMsg;
						lastGCMsg.actorType = GLOBAL_CLOCK;
						lastGCMsg.command = TERMINATE_ACTOR;
						AC_Bcast(&lastGCMsg,AC_GetActorId());
						return AC_TERMINATE_ACTOR;
					}
				}
				else
				{
					// Validates that the execution flow will never come here.
					assert(0);
				}
				break;
			default:
				// Validates that the execution flow will never come here.
				assert(0);
		}
	}



	bool haveMsgsFromCellsArrived	 = (gc.totalMsgsFromCellsThisMonth == (NUM_OF_CELLS));
	bool isThisTheLastMonth 		 = (gc.currentMonth == TOTAL_MONTHS );
	
	/**
	* We move to the next month only if we have received
	* a response from every cell because we need to print 
	* values of the cells for each month.
	**/
	if( ! haveMsgsFromCellsArrived )
	{
		return AC_KEEP_ACTOR_ALIVE;
	}
	gc.totalMsgsFromCellsThisMonth=0;


	
	if(isThisTheLastMonth)
	{
		/** 
		* At the last month, we send a termination message
		* in every actor and we terminate the global clock.
		**/
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