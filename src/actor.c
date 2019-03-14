#include <stdlib.h>
#include <stdio.h>
#include "mpi.h"
#include "pool.h"
#include "actor.h"
#include <assert.h>


int AC_numActors;
int AC_numOfDiffActorTypes;
int* AC_diffActorsQuantity;
void (**AC_functPtrs)();
MPI_Datatype AC_msgDataType;




void AC_Init(int argc, char *argv[])
{
	MPI_Init(&argc,&argv);
}

void actor_Bsend(void* sendBuf,int destActorId)
{
	int ret = MPI_Bsend( sendBuf, 1, AC_msgDataType, destActorId, 0, MPI_COMM_WORLD );
	//MPI_Ssend( sendBuf, 1, AC_msgDataType, destActorId, 0, MPI_COMM_WORLD );
	if(ret != MPI_SUCCESS ) errorMessage("MPI_Bsend failed");
}

MPI_Status actor_Receiv(void* event)
{
	MPI_Status status;
	MPI_Recv( event, 1, AC_msgDataType, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status );
	return status;
}



static void AC_ActorCode()
{
	int actorType=-1;
	MPI_Recv(&actorType,1,MPI_INT,0,7,MPI_COMM_WORLD,MPI_STATUS_IGNORE); 
	assert(actorType != -1);
	for(int i=0; i < AC_numOfDiffActorTypes; i++)
	{
		if(actorType == i)
		{
			(*AC_functPtrs[i])();
		}
	}
	
	
	MPI_Send(NULL, 0, MPI_INT, 0, 0, MPI_COMM_WORLD);
	//printf("111\n");
	int workerStatus = 1;
	while (workerStatus) 
	{
		workerStatus = workerSleep();
	}

	

}

static void AC_PoolMaster()
{
	
	int activeWorkers=0;
	MPI_Request initialWorkerRequests[AC_numActors];
	
	int cnt=0;
	for (int actorType=0; actorType < AC_numOfDiffActorTypes; actorType++)
	{
		for(int j=0; j< AC_diffActorsQuantity[actorType]; j++ )
		{
			int workerPid = startWorkerProcess();	
			activeWorkers++;
			MPI_Irecv(NULL, 0, MPI_INT, workerPid, 0, MPI_COMM_WORLD, &initialWorkerRequests[cnt]);
			
			MPI_Ssend(&actorType,1,MPI_INT,workerPid,7,MPI_COMM_WORLD);
			cnt++;
	
		}
	}
	int masterStatus = masterPoll();
	int returnCode;
	while (masterStatus) {
		masterStatus=masterPoll();
		for (int i=0;i< (AC_numActors);i++) {
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
	
    //MPI_Type_free(&squirrelDataType);


}


void AC_RunSimulation()
{

	int statusCode = processPoolInit();

	if(statusCode == 2) //master
	{

		AC_PoolMaster();
	}
	else if(statusCode == 1)
	{
		AC_ActorCode();
	}

	processPoolFinalise();	

}

void AC_SetActorTypes(int totalActors,int numOfDiffActorTypes,int* quantityForEachType,void (**func_ptr_foreach_actorType)())
{
	int expected_total_actors=0;
	for(int i=0; i<numOfDiffActorTypes; i++)
	{
		expected_total_actors += quantityForEachType[i];
	}
	assert(expected_total_actors == totalActors);
	AC_numActors = totalActors;
	AC_functPtrs = func_ptr_foreach_actorType;
    AC_numOfDiffActorTypes = numOfDiffActorTypes;
    AC_diffActorsQuantity = quantityForEachType;

    //create an array that maps actor type to rank
}

void AC_SetActorMsgDataType(int msgFields, AC_Datatype* msgDataTypeForEachField, int* blockLen,MPI_Aint* disp)
{
	//cell
    MPI_Datatype* actorMsgTypes 	= msgDataTypeForEachField;
    int* actorMsgBlockLen 	  		= blockLen;
    MPI_Aint* actoMsgDisp 			= disp;
    	
	MPI_Type_create_struct(msgFields, actorMsgBlockLen, actoMsgDisp, actorMsgTypes, &AC_msgDataType);
    MPI_Type_commit(&AC_msgDataType);

    int bufSize,msgSize;
    MPI_Pack_size( 1, AC_msgDataType, MPI_COMM_WORLD, &msgSize );
    bufSize = 1000*msgSize;
    int* buf = (int *)malloc( bufSize );
    MPI_Buffer_attach( buf, bufSize );
}


void AC_Finalize()
{
	MPI_Finalize();
}

int AC_GetActorId()
{
	int myRank;
	MPI_Comm_rank(MPI_COMM_WORLD, &myRank);
	return (myRank);
}