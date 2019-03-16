#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "mpi.h"
#include "pool.h"
#include "actor.h"
#include <assert.h>


int AC_numActors;
int AC_msgSizeInBytes;
int AC_numOfDiffActorTypes;
int* AC_diffActorsQuantity;
int (**AC_functPtrs)();
MPI_Datatype AC_msgDataType;




void AC_Init(int argc, char *argv[])
{
	MPI_Init(&argc,&argv);
}

void AC_Bsend(void* sendBuf,int destActorId)
{
	int ret = MPI_Bsend( sendBuf, 1, AC_msgDataType, destActorId, 0, MPI_COMM_WORLD );
	//MPI_Ssend( sendBuf, 1, AC_msgDataType, destActorId, 0, MPI_COMM_WORLD );
	if(ret != MPI_SUCCESS ) errorMessage("MPI_Bsend failed");
}

int AC_Iprobe(int* outstanding)
{
	MPI_Status status;
	MPI_Iprobe(MPI_ANY_SOURCE,0,MPI_COMM_WORLD,outstanding,&status);
	return status.MPI_SOURCE;
}

int AC_Recv(void* event)
{
	MPI_Status status;
	MPI_Recv( event, 1, AC_msgDataType, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status );
	return status.MPI_SOURCE;
}

void AC_Bcast(void* event,int source_actroId)
{
	for(int actroId=1; actroId < AC_numActors; actroId++)
	{
		if(source_actroId == actroId) continue;
		AC_Bsend(event,actroId);
	}
}



static void AC_ActorCode()
{
	int actorType=-1;
	MPI_Recv(&actorType,1,MPI_INT,MPI_ANY_SOURCE,7,MPI_COMM_WORLD,MPI_STATUS_IGNORE); 
			//printf("rrr %d\n",actorType);
	assert(actorType != -1);

	void* msgQueue[AC_MAX_MSG_QUEUE_SIZE];
	void* msg = malloc(AC_msgSizeInBytes);
	for(int i=0; i < AC_MAX_MSG_QUEUE_SIZE; i++)
	{
		msgQueue[i] = malloc(AC_msgSizeInBytes);
	}

	int actorIdQueue[AC_MAX_MSG_QUEUE_SIZE];
	int msgsInQueueCnt			= 0;
	int terminate 				= 0;
	do
	{
		int outstanding =0;
		AC_Iprobe(&outstanding);
		if(!outstanding || msgsInQueueCnt == AC_MAX_MSG_QUEUE_SIZE-1)
		{
			//printf("%------d\n",msgsInQueueCnt );
			
			terminate = (*AC_functPtrs[actorType])(msgQueue,msgsInQueueCnt,actorIdQueue);
			msgsInQueueCnt = 0;
			//terminate = 1;
		}
		else
		{
			int sourceActorId = AC_Recv(msg);
			memcpy((msgQueue[msgsInQueueCnt]),msg,AC_msgSizeInBytes);
			actorIdQueue[msgsInQueueCnt] = sourceActorId;
			msgsInQueueCnt++;
			//printf("+++++%d\n",msgsInQueueCnt );
		}
		
	}while(terminate != 1);


	MPI_Send(NULL, 0, MPI_INT, 0, 0, MPI_COMM_WORLD);
	
	int workerStatus = 1;
	while (workerStatus) 
	{
		workerStatus = workerSleep();
		// worker received wakeup command
		if(workerStatus == 1) 
		{
			// runs again actor code for the worker.
			AC_ActorCode(); 	
		}
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

void AC_SetActorTypes(int totalActors,int numOfDiffActorTypes,int* quantityForEachType,int (**func_ptr_foreach_actorType)())
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
    bufSize = AC_MAX_MSG_QUEUE_SIZE *msgSize;
    int* buf = (int *)malloc( bufSize );
    MPI_Buffer_attach( buf, bufSize );

    AC_msgSizeInBytes =0 ;
    for(int i=0; i<msgFields;i++)
    {
    	AC_msgSizeInBytes += sizeof(msgDataTypeForEachField[i]);
    }
    
}
int AC_GetMSgSize()
{
	return AC_msgSizeInBytes;
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


int AC_CreateNewActor(int actorType)
{
	int workerId = startWorkerProcess();
	MPI_Request childRequest;
	
	//MPI_Irecv(NULL, 0, MPI_INT, workerId, 0, MPI_COMM_WORLD, &childRequest);
	//MPI_Waitall(1, &childRequest, MPI_STATUS_IGNORE);
	//printf("xxxxxxx\n");
	//printf("====%d\n",workerId);
	MPI_Ssend(&actorType,1,MPI_INT,workerId,7,MPI_COMM_WORLD);


	return workerId;
}
