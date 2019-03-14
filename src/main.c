#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stddef.h>
#include "squirrel-functions.h"
#include "mpi.h"
#include "pool.h"
#include <assert.h>
#define TOTALMONTHS 24
#define MPI_ACTORS_WORLD 1234


const int numOfCells = 16;
const int maxNumOfSquirrels=200;
const int numOfSquirrels=34;
const int initialInfectionLevel=4;
int currentMonth=0;

int myRank;
long state;
MPI_Datatype squirrelDataType;
MPI_Datatype cellDataType;
MPI_Datatype actorMsgDataType;
MPI_Datatype squirrelStepInfoDataType;
MPI_Datatype cellReplyInfoDataType;

typedef enum squirrelState
{
	INFECTED=0,
	DEAD=1,
	HEALTHY=2
}squirrelState;


typedef struct task
{
	int numberOfStepsPerMonth[TOTALMONTHS];
}task;

typedef struct squirrel
{
	squirrelState state;
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


enum actorType
{
	MASTER_OF_POOL=0,
	GLOBAL_CLOCK=1,
	SQUIRREL=2,
	CELL=3
};

typedef struct actorMsg
{
	enum actorType type;	
	//actorData data;
}actorMsg;

typedef enum globalClockCommands
{
	START=1,
	NEXT_MONTH=2
}globalClockCommands;


void initSimulation(squirrel squirrels[],cell cells[])
{

	float x=0,y=0;
	int cell;
	for(int i=0; i < numOfSquirrels; i++)
	{
		if( i>=0 && i<4)
		{
			squirrels[i].state = INFECTED;
		}else
		{
			squirrels[i].state = HEALTHY;	
		}
		
		squirrelStep(0,0,&x,&y,&state);
		squirrels[i].cell = getCellFromPosition(x,y);
		squirrels[i].x = x;
		squirrels[i].y = y;
		/*printf("squirel %d x = %f, y = %f,cell = %d, totalSteps = %d, remainingSteps = %d, isInfected = %d, isAlive = %d\n",
			i,
			squirrels[i].x,
			squirrels[i].y,
			squirrels[i].cell,
			squirrels[i].totalSteps,
			squirrels[i].remainingSteps,
			squirrels[i].isInfected,
			squirrels[i].isAlive
		);*/
	}

	for(int i=0; i < numOfCells; i++){		
		cells[i].populationInflux = 0 ;
		cells[i].infectionLevel = 0 ;
	}	

	for(int i=0; i < numOfCells; i++)
	{
		cells[i].infectionLevel = 0;
		for(int j=0; j < numOfSquirrels; j++)	
		{
			if(squirrels[j].cell == i)
			{
				cells[i].populationInflux = cells[i].populationInflux +1; 
				//printf("%d\n", cells[i].totalPopulationInflux);
			}
		}
	}


}



int waitGlobalClockBeforeStart()
{
	MPI_Status status;
	int command = -1;
	MPI_Recv(&command,1,MPI_INT,MPI_ANY_SOURCE,0,MPI_COMM_WORLD,&status);
	

	if(command == myRank)
	{
		//printf("actor %d starts from %d\n",myRank,status.MPI_SOURCE);
	}
	return status.MPI_SOURCE;
}


void doStep(squirrel sq,int cell_id_to_rank[])
{
	squirrelStep(sq.x,sq.y,&(sq.x),&(sq.y),&state);
	int nextCell = getCellFromPosition(sq.x,sq.y);
	sq.cell =nextCell;

	MPI_Bsend(&(sq.state),1,MPI_INT,cell_id_to_rank[sq.cell],0,MPI_COMM_WORLD );
	printf("squirrel %d step to process %d \n",myRank,cell_id_to_rank[sq.cell]);

}

void squirrelCode()
{
	MPI_Status status;
	int cellToProc[numOfCells];
	int totalSteps = 0;
	int s1;
    MPI_Pack_size( 1, MPI_INT, MPI_COMM_WORLD, &s1 );

	squirrel sq;
	MPI_Recv(&sq,1,squirrelDataType,0,0,MPI_COMM_WORLD,&status); 			// receives squirrel data
	//printf("Squirrel totalSteps=%d \n",sq.totalSteps);
	MPI_Recv(&cellToProc,numOfCells,MPI_INT,0,0,MPI_COMM_WORLD,&status); 	// receives cell to mpi process mapping 
	
	int globalClockRank = waitGlobalClockBeforeStart();

	do
	{
		int outstanding =0;
		MPI_Iprobe(MPI_ANY_SOURCE,0,MPI_COMM_WORLD,&outstanding,&status);
		if(!outstanding)
		{

			//printf("squirrel computations\n");
			//doStep(sq,cellToProc);
			//totalSteps++;


			if(totalSteps == 50)
			{
				//printf("sq %d steps to cell %d\n",myRank,sq.cell);
				totalSteps = 0;
			}

		}
		else
		{
			if(status.MPI_SOURCE == globalClockRank)
			{
				int month = -1;
				MPI_Recv(&month,1,MPI_INT,globalClockRank,0,MPI_COMM_WORLD,&status);
				assert(month!=-1);
				assert(month == currentMonth+1);
				currentMonth = month;
				//printf("squirrel %d received CM command (Month: %d)\n",myRank,currentMonth);		
				MPI_Ssend(&(sq.state),1,MPI_INT,globalClockRank,0,MPI_COMM_WORLD);
			}

		}
	}while(currentMonth != TOTALMONTHS);

	
	//printf("squirrel %d make a step to cell %d\n",myRank,cellToProc[sq.cell]);
	
	//printf("squirrel %d prev_x=%f, prev_y=%f, prev_cell=%d\n",myRank,sq.x,sq.y,sq.cell);
	
	//printf("squirrel %d next_x=%f, next_y=%f, next_cell=%d \n",myRank,sq.x,sq.y,sq.cell);

	
	//MPI_Ssend(&step,1,MPI_INT,cellToProc[sq.cell],0,MPI_COMM_WORLD);
}

void recvSquirrel()
{
	int squirrelState = -1;
	MPI_Status status;
	MPI_Request req;
	MPI_Recv(&squirrelState,1,MPI_INT,MPI_ANY_SOURCE,0,MPI_COMM_WORLD,&status);
	printf("cell %d received a squirel\n",myRank);
	assert(squirrelState !=-1);
}

void cellCode()
{
	MPI_Status status;
	cell cell;
	int populationInflux_Monthly[TOTALMONTHS];
	double infectionLevel_Monthly[TOTALMONTHS];
	int populationInflux = -1;
	int infectionLevel=-2;


	for(int i=0; i < TOTALMONTHS; i++)
	{
		populationInflux_Monthly[i] = 0;
		infectionLevel_Monthly[i] =0;
	}
	
	MPI_Recv(&cell,1,cellDataType,0,0,MPI_COMM_WORLD,&status);
	assert(currentMonth == 0);
	populationInflux_Monthly[currentMonth] = cell.populationInflux;
	populationInflux_Monthly[currentMonth] = cell.infectionLevel;
	//printf("Cell %d totalSquirrels = %d\n",myRank,cell.numOfSquirrels[0]);
	int globalClockRank = waitGlobalClockBeforeStart();
	
	do
	{
		int outstanding =0;
		MPI_Iprobe(MPI_ANY_SOURCE,0,MPI_COMM_WORLD,&outstanding,&status);
		if(!outstanding)
		{
			//printf("cell computations\n");
			//recvSquirrel();
		}
		else
		{
			if(status.MPI_SOURCE == globalClockRank)
			{
				int month = -1;
				MPI_Recv(&month,1,MPI_INT,globalClockRank,0,MPI_COMM_WORLD,&status);
				assert(month!=-1);
				assert(month == currentMonth+1);
				currentMonth = month;			
				
				int cellData[2];			
				cellData[0] = populationInflux;
				cellData[1] = infectionLevel;
				MPI_Ssend(&(cellData[0]),2,MPI_INT,globalClockRank,0,MPI_COMM_WORLD);
			}
			
			//printf("cell %d received CM command (Month: %d)\n",myRank,currentMonth);		
		}
	}while(currentMonth != TOTALMONTHS);

	//MPI_Recv(&rank,1,MPI_INT,MPI_ANY_SOURCE,0,MPI_COMM_WORLD,&status);
	//printf("cell %d receives squirrel %d\n", myRank,rank);
	
	//sleep(1);
}

void startActors(int cellToProc[],int squirrelsRank[] )
{
	MPI_Request request;
	for(int i=0; i < numOfSquirrels; i++)
	{
		MPI_Isend(&(squirrelsRank[i]),1,MPI_INT,squirrelsRank[i],0,MPI_COMM_WORLD,&request);

	}
	for(int i=0; i < numOfCells; i++)
	{
		MPI_Isend(&(cellToProc[i]),1,MPI_INT,cellToProc[i],0,MPI_COMM_WORLD,&request);
	}

}
void changeActorsMonth(int cellToProc[],int squirrelsRank[] )
{
	MPI_Request request;
	currentMonth = currentMonth+1;
	for(int i=0; i < numOfSquirrels; i++)
	{
		MPI_Isend(&(currentMonth),1,MPI_INT,squirrelsRank[i],0,MPI_COMM_WORLD,&request);

	}
	
	for(int i=0; i < numOfCells; i++)
	{
		MPI_Isend(&(currentMonth),1,MPI_INT,cellToProc[i],0,MPI_COMM_WORLD,&request);
	}
}

void getActorsMonthlyData(int cell_id_to_rank[],int squirrel_id_to_rank[])
{
	int cellData[numOfCells][2];
	MPI_Status status;

	/*receives populationInflux and infectionLevel of each cell|*/
	for(int i=0;i < numOfCells; i++) 
	{
		MPI_Recv(&(cellData[i]), 2, MPI_INT,cell_id_to_rank[i],0,MPI_COMM_WORLD,&status);
		//printf("populationInflux=%d infectionLevel=%d\n",cellData[i][0], cellData[i][1]);
	}

	
	int squirrelsState[numOfSquirrels];
	MPI_Request requests[numOfSquirrels];
	MPI_Status statuses[numOfSquirrels];
	int totalInfectedSquirrels =0;
	int totalHealthySquirrels = 0;
	
	for(int i=0; i < numOfSquirrels; i++)
	{
		MPI_Irecv(&squirrelsState[i],1,MPI_INT,squirrel_id_to_rank[i],0,MPI_COMM_WORLD,&requests[i]);

	}
	MPI_Waitall(numOfSquirrels,requests,statuses);

	for(int i=0; i < numOfSquirrels; i++)
	{
		if(squirrelsState[i] == HEALTHY)
		{
			totalHealthySquirrels++;
		}else if(squirrelsState[i] == INFECTED)
		{
			totalInfectedSquirrels++;
		}else{
			//errorMessage("Not acceptable state yet\n");
		}	
	}
	printf("Current Month is %d\n",currentMonth );
	printf("total number of infected squirrels is %d\n",totalInfectedSquirrels );
	printf("total number of healthy squirrels is %d\n",totalHealthySquirrels );
	printf("Num of cell \t  populationInflux \t InfectionLevel\n");
	for(int i=0;i < numOfCells; i++) 
	{
		printf("%d \t\t %d \t\t\t %d\n",i,cellData[i][0],cellData[i][1] );
	}


}

void globalClockCode()
{
	MPI_Status status;
	int cellToProc[numOfCells];
	int squirrelsRank[numOfSquirrels];
	
	MPI_Recv(&cellToProc,numOfCells,MPI_INT,0,0,MPI_COMM_WORLD,&status); 			// receives cells ranks
	MPI_Recv(&squirrelsRank,numOfSquirrels,MPI_INT,0,0,MPI_COMM_WORLD,&status); 	// receives squirrels ranks
	

	startActors(cellToProc,squirrelsRank);
	
	for(int i=0; i < TOTALMONTHS; i++)
	{
		sleep(0.5);
		changeActorsMonth(cellToProc,squirrelsRank);
		getActorsMonthlyData(cellToProc,squirrelsRank);
	}

	
}

int actorCode()
{
	MPI_Status status;
	int cellToProc[numOfCells];

	actorMsg actorPkg;
	MPI_Recv(&actorPkg, 1, actorMsgDataType, 0, 0, MPI_COMM_WORLD,&status);
	if(actorPkg.type == SQUIRREL)
	{
		squirrelCode();
	} 
	else if(actorPkg.type == CELL)
	{
		cellCode();
	}
	else if(actorPkg.type == GLOBAL_CLOCK)
	{
		
		globalClockCode();
	} 
	MPI_Ssend(NULL, 0, MPI_INT, 0, 0, MPI_COMM_WORLD);


	int workerStatus = 1;
	while(workerStatus)
	{
		workerStatus = workerSleep();

	}
}

void initDatatypes()
{

	//squirrel
    MPI_Datatype squirrelTypes[6] = {  
			    						MPI_INT,
			    						MPI_FLOAT,
			    						MPI_FLOAT,
			    						MPI_INT, 
			    						MPI_INT,
			    						MPI_INT 
    								};
    int squirrelBlockLen[6] = { 1,1,1,1,1,1 };
    MPI_Aint squirrelDisp[6];
 	squirrelDisp[0] = offsetof(squirrel, state);
	squirrelDisp[1] = offsetof(squirrel, x);
	squirrelDisp[2] = offsetof(squirrel, y);
	squirrelDisp[3] = offsetof(squirrel, cell);

	squirrelDisp[4] = offsetof(squirrel, totalPopulationInflux);
	squirrelDisp[5] = offsetof(squirrel, totalInfectionLevel);
    
    MPI_Type_create_struct(6, squirrelBlockLen, squirrelDisp, squirrelTypes, &squirrelDataType);
    MPI_Type_commit(&squirrelDataType);

    //cell
    MPI_Datatype cellTypes[2] = {MPI_INT,MPI_DOUBLE};
    int cellBlockLen[2] 	  = {1,1};
    MPI_Aint cellDisp[2];
    cellDisp[0]=offsetof(cell,populationInflux);
	cellDisp[1]=offsetof(cell,infectionLevel);

	MPI_Type_create_struct(2, cellBlockLen, cellDisp, cellTypes, &cellDataType);
    MPI_Type_commit(&cellDataType);

    //actor control 
    MPI_Datatype actorMsgType[1] = {MPI_INT};
    int actorMsgBlockLen[1]		= {1};
    MPI_Aint actorMsgDisp[1];
    actorMsgDisp[0] 				= offsetof(actorMsg,type);
    MPI_Type_create_struct(1,actorMsgBlockLen , actorMsgDisp, actorMsgType, &actorMsgDataType);
    MPI_Type_commit(&actorMsgDataType);

}

int main(int argc, char *argv[])
{
	MPI_Init(&argc,&argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &myRank);
	long seed = -1-myRank;
	initialiseRNG(&seed);
	initDatatypes();
	MPI_Status status;

	

	/*
	state = seed;
	squirrel sq;
	if(myRank == 0)
	{
		sq.x = 77;
		MPI_Ssend(&(sq),1,squirrelDataType,1,0,MPI_COMM_WORLD);
	}else{
		MPI_Recv(&sq, 1, squirrelDataType, 0, 0, MPI_COMM_WORLD,&status);
		printf("sq.x = %f\n",sq.x);
	}*/



	int statusCode = processPoolInit();


	if(statusCode == 2) //master
	{
		cell 		grid[numOfCells];
		squirrel 	squirrels[numOfSquirrels];
		int 		cellToProc[numOfCells];
		int 		squirrelsRank[numOfSquirrels];
		initDatatypes(squirrels,grid);
		initSimulation(squirrels,grid);

		/*int numProcs;
		MPI_Comm_size(MPI_COMM_WORLD, &numProcs);
		if(numProcs < maxNumOfSquirrels+numOfCells+1)
		{
			errorMessage("Not enough processes.");
		}*/
		int i, activeWorkers=0, returnCode;
		MPI_Request initialWorkerRequests[numOfSquirrels+numOfCells+1];
		MPI_Request requests[numOfSquirrels];
		MPI_Status status;
		actorMsg actorPkg;

		// wake up the cells
		for (i=0; i < numOfCells; i++) 
		{
								
			int workerPid = startWorkerProcess();
			cellToProc[i] = workerPid; 
			//printf("workerPid = %d\n",workerPid);	
			MPI_Irecv(NULL, 0, MPI_INT, workerPid, 0, MPI_COMM_WORLD, &initialWorkerRequests[i]); ///
			activeWorkers++;
			actorPkg.type = CELL;
			//fprintf(stderr,"---cellToProc[%d]=%d\n",i,workerPid);

			MPI_Ssend(&actorPkg,1,actorMsgDataType,workerPid,0,MPI_COMM_WORLD);
			MPI_Ssend(&(grid[i]),1,cellDataType,workerPid,0,MPI_COMM_WORLD);
		}

		// wake up the squirrels
		int cnt=0;
		for (i=0; i < numOfSquirrels; i++) 
		{
			int workerPid = startWorkerProcess();
			MPI_Irecv(NULL, 0, MPI_INT, workerPid, 0, MPI_COMM_WORLD, &initialWorkerRequests[i+numOfCells]); /// 
			//printf("workerPid = %d\n",workerPid);						
			activeWorkers++;
			//printf("Master started worker %d on MPI process %d\n", i , workerPid);
			
			actorPkg.type = SQUIRREL; 
			MPI_Ssend(&actorPkg,1,actorMsgDataType,workerPid,0,MPI_COMM_WORLD);
			MPI_Ssend(&(squirrels[i]),1,squirrelDataType,workerPid,0,MPI_COMM_WORLD);

			MPI_Ssend(&cellToProc[0],numOfCells,MPI_INT,workerPid,0,MPI_COMM_WORLD);

			squirrelsRank[cnt++] = workerPid;
		}
		//printf("----Actors Initialization finished\n" );

		for (i=0; i< 1; i++)
		{
			int workerPid = startWorkerProcess();
			MPI_Irecv(NULL, 0, MPI_INT, workerPid, 0, MPI_COMM_WORLD, &initialWorkerRequests[i+numOfCells+numOfSquirrels]); /// 
			activeWorkers++;

			actorPkg.type = GLOBAL_CLOCK;
			MPI_Ssend(&actorPkg,1,actorMsgDataType,workerPid,0,MPI_COMM_WORLD);
			MPI_Ssend(&cellToProc[0],numOfCells,MPI_INT,workerPid,0,MPI_COMM_WORLD);
			MPI_Ssend(&squirrelsRank[0],numOfSquirrels,MPI_INT,workerPid,0,MPI_COMM_WORLD);
		}


		int masterStatus = masterPoll();
		while (masterStatus) {
			masterStatus=masterPoll();

			for (i=0;i< (numOfCells+numOfSquirrels+1);i++) {
				// Checks all outstanding workers that master spawned to see if they have completed
				if (initialWorkerRequests[i] != MPI_REQUEST_NULL) {
					MPI_Test(&initialWorkerRequests[i], &returnCode, MPI_STATUS_IGNORE);
					if (returnCode){
						activeWorkers--;	
						//printf("++++++++++Active workers now are %d \n",activeWorkers);
					} 

				}
			}
			// If we have no more active workers then quit poll loop which will effectively shut the pool down when  processPoolFinalise is called
			if (activeWorkers==0)
			{
				break;
			}
		}
		
	}else if(statusCode== 1){ 
		actorCode();
	}

	processPoolFinalise();
	
    MPI_Type_free(&squirrelDataType);
	MPI_Finalize();
	return 0;
}


