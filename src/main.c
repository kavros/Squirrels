#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stddef.h>
#include "squirrel-functions.h"
#include "mpi.h"
#include "pool.h"
#define  TOTALMONTHS 24

const int numOfCells = 16;
const int maxNumOfSquirrels=200;
const int numOfSquirrels=34;
const double initialInfectionLevelForACell=4.0f/16.0f;
const int currentMonth=1;

int myRank;
long state;
MPI_Datatype squirrelDataType;
MPI_Datatype cellDataType;
MPI_Datatype actorControlDataType;

enum actorControlCommand
{
	MASTER_OF_POOL=0,
	GLOBAL_CLOCK=1,
	SQUIRREL=2,
	CELL=3
};

typedef struct actorControlPackage
{
	enum actorControlCommand type;
	//void* data;
}actorControlPackage;

typedef struct cell
{
	int numOfSquirrels[TOTALMONTHS];			// total number of squirrels in the cell for each month.
	int numOfInfectedSquirrels[TOTALMONTHS];	// total number of infected squirrels for each month.	
}cell;

typedef struct squirrel
{
	bool isInfected;
	bool isAlive;
	float x;
	float y;
	int cell;
	int totalSteps;
	int remainingSteps;
	
	int totalPopulationInflux; 		// totalpopulationInflux for the last 50 steps
	int totalInfectionLevel;		// totalInfectionLevel for the last 50 steps
}squirrel;

typedef struct task
{
	int numberOfStepsPerMonth[TOTALMONTHS];
}task;

void initSimulation(squirrel squirrels[],cell cells[],task bag[])
{

	float x=0,y=0;
	int cell;
	for(int i=0; i < numOfSquirrels; i++)
	{
		squirrels[i].isInfected = false;
		squirrels[i].isAlive = true;
		squirrelStep(0,0,&x,&y,&state);
		squirrels[i].cell = getCellFromPosition(x,y);
		squirrels[i].x = x;
		squirrels[i].y = y;
		squirrels[i].totalSteps=5;
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

				cells[i].numOfSquirrels[currentMonth-1] =cells[i].numOfSquirrels[currentMonth-1]+1;
			}
		}
		cells[i].numOfInfectedSquirrels[currentMonth-1] = 0;
		//printf("cell %d,squirrels = %d \n",i,cells[i].numOfSquirrels[currentMonth-1]);
		//printf("cell %d,infected squirrels = %d\n",i,cells[i].numOfInfectedSquirrels[currentMonth-1]);
	}
}

void errorMessage(char * message) {
	fprintf(stderr,"%4d: [ProcessPool] %s\n", myRank, message);
	MPI_Abort(MPI_COMM_WORLD, 1);
}

int actorCode()
{
	MPI_Status status;
	int cellToProc[numOfCells];

	actorControlPackage actorPkg;
	MPI_Recv(&actorPkg, 1, actorControlDataType, 0, 0, MPI_COMM_WORLD,&status);
	if(actorPkg.type == SQUIRREL)
	{
		squirrel sq;
		MPI_Recv(&sq,1,squirrelDataType,0,0,MPI_COMM_WORLD,&status); 			// receives squirrel data
		//printf("Squirrel totalSteps=%d \n",sq.totalSteps);
		MPI_Recv(&cellToProc,numOfCells,MPI_INT,0,0,MPI_COMM_WORLD,&status); 	// receives cell to mpi process mapping 
		
		/*for(int i=0; i<numOfCells; i++)
		{
			printf(" (%d,%d)",i,cellToProc[i]);
		}
		printf("\n" );*/
		//printf("squirrel %d make a step to cell %d\n",myRank,cellToProc[sq.cell]);
		int step= myRank;
		MPI_Ssend(&step,1,MPI_INT,cellToProc[sq.cell],0,MPI_COMM_WORLD);

	} else if(actorPkg.type == CELL)
	{
		cell cell;
		MPI_Recv(&cell,1,cellDataType,0,0,MPI_COMM_WORLD,&status);
		//printf("Cell %d totalSquirrels = %d\n",myRank,cell.numOfSquirrels[0]);

		int rank = -1;
		for(int i=0; i < cell.numOfSquirrels[0]; i++)
		{
			MPI_Recv(&rank,1,MPI_INT,MPI_ANY_SOURCE,0,MPI_COMM_WORLD,&status);
			//printf("cell %d receives squirrel %d\n", myRank,rank);
		}
		//sleep(1);
		
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
    						MPI_C_BOOL,
    						MPI_C_BOOL,
    						MPI_FLOAT,
    						MPI_FLOAT,
    						MPI_INT, 
    						MPI_INT, 
    						MPI_INT, 
    						MPI_INT,
    						MPI_INT 
    						};
    int squirrelBlockLen[9] = { 1,1,1,1,1,1,1,1,1 };
    MPI_Aint squirrelDisp[9];
 
    
 	squirrelDisp[0] = offsetof(squirrel, isInfected);
 	squirrelDisp[1] = offsetof(squirrel, isAlive);
	squirrelDisp[2] = offsetof(squirrel, x);
	squirrelDisp[3] = offsetof(squirrel, y);
	squirrelDisp[4] = offsetof(squirrel, cell);
	squirrelDisp[5] = offsetof(squirrel, totalSteps);
	squirrelDisp[6] = offsetof(squirrel, remainingSteps);
	squirrelDisp[7] = offsetof(squirrel, totalPopulationInflux);
	squirrelDisp[8] = offsetof(squirrel, totalInfectionLevel);
    
    MPI_Type_create_struct(9, squirrelBlockLen, squirrelDisp, squirrelTypes, &squirrelDataType);
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
    int actorControlPackageBlockLen[1]={1};
    MPI_Aint actorControlPackageDisp[1];
    actorControlPackageDisp[0] = offsetof(actorControlPackage,type);
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
		initDatatypes(squirrels,grid);
		initSimulation(squirrels,grid,bag);

		/*int numProcs;
		MPI_Comm_size(MPI_COMM_WORLD, &numProcs);
		if(numProcs < maxNumOfSquirrels+numOfCells+1)
		{
			errorMessage("Not enough processes.");
		}*/
		int i, activeWorkers=0, returnCode;
		MPI_Request initialWorkerRequests[numOfSquirrels+numOfCells];
		MPI_Request requests[numOfSquirrels];
		MPI_Status status;
		actorControlPackage actorPkg;


		for (i=0; i < numOfCells; i++) 
		{
								
			int workerPid = startWorkerProcess();
			cellToProc[i] = workerPid; 
			//printf("workerPid = %d\n",workerPid);	
			MPI_Irecv(NULL, 0, MPI_INT, workerPid, 0, MPI_COMM_WORLD, &initialWorkerRequests[i]); /// ??????
			activeWorkers++;
			actorPkg.type = CELL;
			//fprintf(stderr,"---cellToProc[%d]=%d\n",i,workerPid);

			MPI_Ssend(&actorPkg,1,actorControlDataType,workerPid,0,MPI_COMM_WORLD);
			MPI_Ssend(&(grid[i]),1,cellDataType,workerPid,0,MPI_COMM_WORLD);

			
		}

		for (i=0; i < numOfSquirrels; i++) 
		{
			int workerPid = startWorkerProcess();
			MPI_Irecv(NULL, 0, MPI_INT, workerPid, 0, MPI_COMM_WORLD, &initialWorkerRequests[i+numOfCells]); /// ??????
			//printf("workerPid = %d\n",workerPid);						
			activeWorkers++;
			//printf("Master started worker %d on MPI process %d\n", i , workerPid);
			
			actorPkg.type = SQUIRREL; 
			MPI_Ssend(&actorPkg,1,actorControlDataType,workerPid,0,MPI_COMM_WORLD);
			MPI_Ssend(&(squirrels[i]),1,squirrelDataType,workerPid,0,MPI_COMM_WORLD);

			MPI_Ssend(&cellToProc[0],numOfCells,MPI_INT,workerPid,0,MPI_COMM_WORLD);
		}
		//printf("----Actors Initialization finished\n" );

		int masterStatus = masterPoll();
		
		while (masterStatus) {
			masterStatus=masterPoll();

			for (i=0;i< (numOfCells+numOfSquirrels);i++) {
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


