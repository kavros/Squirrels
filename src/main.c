#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stddef.h>
#include "squirrel-functions.h"
#include "mpi.h"
#include "pool.h"
#define TOTALMONTHS 24
#define MPI_ACTORS_WORLD 1234


const int numOfCells = 16;
const int maxNumOfSquirrels=200;
const int numOfSquirrels=4;
const double initialInfectionLevelForACell=4.0f/16.0f;
int currentMonth=0;

int myRank;
long state;
MPI_Datatype squirrelDataType;
MPI_Datatype cellDataType;
MPI_Datatype actorControlDataType;
MPI_Datatype squirrelStepInfoDataType;
MPI_Datatype cellReplyInfoDataType;

typedef enum squirrelState
{
	INFECTED=0,
	DEAD=1,
	GIVING_BIRTH=2,
	GIVING_BIRTH_AND_INFECTED=3,
	HEALTHY
}squirrelState;


typedef struct task
{
	int numberOfStepsPerMonth[TOTALMONTHS];
}task;

typedef struct squirrel
{
	//bool isInfected;
	//bool isAlive;
	squirrelState state;
	float x;
	float y;
	int cell;
	int totalSteps;
	int remainingSteps;
	
	int totalPopulationInflux; 		// totalpopulationInflux for the last 50 steps
	int totalInfectionLevel;		// totalInfectionLevel for the last 50 steps
}squirrel;

typedef struct cell
{
	int numOfSquirrels[TOTALMONTHS];			// total number of squirrels in the cell for each month.
	int numOfInfectedSquirrels[TOTALMONTHS];	// total number of infected squirrels for each month.	
}cell;


enum actorType
{
	MASTER_OF_POOL=0,
	GLOBAL_CLOCK=1,
	SQUIRREL=2,
	CELL=3
};


/*
typedef struct actorData
{
	cell cell;
	squirrel sq;		// squirrel data
}actorData;
*/
typedef struct actorControlPackage
{
	enum actorType type;
	//actorData data;
}actorControlPackage;

typedef enum globalClockCommands
{
	START=1,
	NEXT_MONTH=2
}globalClockCommands;


void initSimulation(squirrel squirrels[],cell cells[],task bag[])
{

	float x=0,y=0;
	int cell;
	for(int i=0; i < numOfSquirrels; i++)
	{
		squirrels[i].state = HEALTHY;
		squirrelStep(0,0,&x,&y,&state);
		squirrels[i].cell = getCellFromPosition(x,y);
		squirrels[i].x = x;
		squirrels[i].y = y;
		squirrels[i].totalSteps=100;
		squirrels[i].remainingSteps=squirrels[i].totalSteps;
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
		for(int month=0; month < TOTALMONTHS; month++)
		{
			cells[i].numOfSquirrels[month] = 0 ;
			cells[i].numOfInfectedSquirrels[month] = 0 ;
		}
	}

	for(int i=0; i < numOfCells; i++)
	{

		for(int j=0; j < numOfSquirrels; j++)	
		{
			if(squirrels[j].cell == i)
			{

				cells[i].numOfSquirrels[currentMonth] =cells[i].numOfSquirrels[currentMonth]+1;
			}
		}
		cells[i].numOfInfectedSquirrels[currentMonth] = 0;
		//printf("cell %d,squirrels = %d \n",i,cells[i].numOfSquirrels[currentMonth-1]);
		//printf("cell %d,infected squirrels = %d\n",i,cells[i].numOfInfectedSquirrels[currentMonth-1]);
	}
}

void errorMessage(char * message) {
	fprintf(stderr,"%4d: [ProcessPool] %s\n", myRank, message);
	MPI_Abort(MPI_COMM_WORLD, 1);
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



void squirrelCode()
{
	MPI_Status status;
	int cellToProc[numOfCells];

	squirrel sq;
	MPI_Recv(&sq,1,squirrelDataType,0,0,MPI_COMM_WORLD,&status); 			// receives squirrel data
	//printf("Squirrel totalSteps=%d \n",sq.totalSteps);
	MPI_Recv(&cellToProc,numOfCells,MPI_INT,0,0,MPI_COMM_WORLD,&status); 	// receives cell to mpi process mapping 
	
	int globalClockRank = waitGlobalClockBeforeStart();

	do
	{
		int outstanding =0;
		MPI_Iprobe(globalClockRank,0,MPI_COMM_WORLD,&outstanding,&status);
		if(!outstanding)
		{
			//printf("squirrel computations\n");
		}
		else
		{
			int month = -1;
			MPI_Recv(&month,1,MPI_INT,globalClockRank,0,MPI_COMM_WORLD,&status);
			currentMonth = month;
			//printf("squirrel %d received CM command (Month: %d)\n",myRank,currentMonth);		


			MPI_Ssend(&(sq.state),1,MPI_INT,globalClockRank,0,MPI_COMM_WORLD);

		}
	}while(currentMonth != TOTALMONTHS);

	
	//printf("squirrel %d make a step to cell %d\n",myRank,cellToProc[sq.cell]);
	
	//printf("squirrel %d prev_x=%f, prev_y=%f, prev_cell=%d\n",myRank,sq.x,sq.y,sq.cell);
	squirrelStep(sq.x,sq.y,&(sq.x),&(sq.y),&state);
	int nextCell = getCellFromPosition(sq.x,sq.y);
	sq.cell =nextCell;
	//printf("squirrel %d next_x=%f, next_y=%f, next_cell=%d \n",myRank,sq.x,sq.y,sq.cell);

	
	//MPI_Ssend(&step,1,MPI_INT,cellToProc[sq.cell],0,MPI_COMM_WORLD);
}

void cellCode()
{
	MPI_Status status;
	cell cell;
	int populationInflux = -1;
	int infectionLevel=-2;

	MPI_Recv(&cell,1,cellDataType,0,0,MPI_COMM_WORLD,&status);

	//printf("Cell %d totalSquirrels = %d\n",myRank,cell.numOfSquirrels[0]);
	int globalClockRank = waitGlobalClockBeforeStart();
	
	do
	{
		int outstanding =0;
		MPI_Iprobe(globalClockRank,0,MPI_COMM_WORLD,&outstanding,&status);
		if(!outstanding)
		{
			//printf("cell computations\n");

		}
		else
		{
			int month = -1;
			MPI_Recv(&month,1,MPI_INT,globalClockRank,0,MPI_COMM_WORLD,&status);
			currentMonth = month;


			int cellData[2];			

			cellData[0] = populationInflux;
			cellData[1] = infectionLevel;

			MPI_Ssend(&(cellData[0]),2,MPI_INT,globalClockRank,0,MPI_COMM_WORLD);

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

	
	int squirrelState;
	int totalInfectedSquirrels =0;
	int totalHealthySquirrels = 0;
	
	for(int i=0; i < numOfSquirrels; i++)
	{
		MPI_Recv(&squirrelState,1,MPI_INT,squirrel_id_to_rank[i],0,MPI_COMM_WORLD,&status);
		if(squirrelState == HEALTHY)
		{
			totalHealthySquirrels++;
		}else if(squirrelState == INFECTED)
		{
			totalInfectedSquirrels++;
		}else{
			errorMessage("Not acceptable state yet\n");
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
	
	MPI_Recv(&cellToProc,numOfCells,MPI_INT,0,0,MPI_COMM_WORLD,&status); 	// receives [ cell to mpi process mapping ]
	MPI_Recv(&squirrelsRank,numOfSquirrels,MPI_INT,0,0,MPI_COMM_WORLD,&status); 	// receives cell to mpi process mapping 
	
	printf(" I am Global Clock!\n");
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

	actorControlPackage actorPkg;
	MPI_Recv(&actorPkg, 1, actorControlDataType, 0, 0, MPI_COMM_WORLD,&status);
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
    MPI_Datatype squirrelTypes[9] = {  
			    						MPI_INT,
			    						MPI_FLOAT,
			    						MPI_FLOAT,
			    						MPI_INT, 
			    						MPI_INT, 
			    						MPI_INT, 
			    						MPI_INT,
			    						MPI_INT 
    								};
    int squirrelBlockLen[8] = { 1,1,1,1,1,1,1,1 };
    MPI_Aint squirrelDisp[8];
 
    
 	squirrelDisp[0] = offsetof(squirrel, state);
	squirrelDisp[1] = offsetof(squirrel, x);
	squirrelDisp[2] = offsetof(squirrel, y);
	squirrelDisp[3] = offsetof(squirrel, cell);
	squirrelDisp[4] = offsetof(squirrel, totalSteps);
	squirrelDisp[5] = offsetof(squirrel, remainingSteps);
	squirrelDisp[6] = offsetof(squirrel, totalPopulationInflux);
	squirrelDisp[7] = offsetof(squirrel, totalInfectionLevel);
    
    MPI_Type_create_struct(8, squirrelBlockLen, squirrelDisp, squirrelTypes, &squirrelDataType);
    MPI_Type_commit(&squirrelDataType);

    //cell
    MPI_Datatype cellTypes[2] = {MPI_INT,MPI_INT};
    int cellBlockLen[2] 	  = {TOTALMONTHS,TOTALMONTHS};
    MPI_Aint cellDisp[2];
    cellDisp[0]=offsetof(cell,numOfSquirrels);
	cellDisp[1]=offsetof(cell,numOfInfectedSquirrels);

	MPI_Type_create_struct(2, cellBlockLen, cellDisp, cellTypes, &cellDataType);
    MPI_Type_commit(&cellDataType);

    //actor control 
    MPI_Datatype actorControlPackageType[1] = {MPI_INT};
    int actorControlPackageBlockLen[1]		= {1};
    MPI_Aint actorControlPackageDisp[1];
    actorControlPackageDisp[0] 				= offsetof(actorControlPackage,type);
    MPI_Type_create_struct(1,actorControlPackageBlockLen , actorControlPackageDisp, actorControlPackageType, &actorControlDataType);
    MPI_Type_commit(&actorControlDataType);

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
		task		bag[TOTALMONTHS];
		int 		cellToProc[numOfCells];
		int 		squirrelsRank[numOfSquirrels];
		initDatatypes(squirrels,grid);
		initSimulation(squirrels,grid,bag);

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
		actorControlPackage actorPkg;

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

			MPI_Ssend(&actorPkg,1,actorControlDataType,workerPid,0,MPI_COMM_WORLD);
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
			MPI_Ssend(&actorPkg,1,actorControlDataType,workerPid,0,MPI_COMM_WORLD);
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
			MPI_Ssend(&actorPkg,1,actorControlDataType,workerPid,0,MPI_COMM_WORLD);
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


