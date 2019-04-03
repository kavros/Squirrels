#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "mpi.h"
#include "actor.h"
#include <assert.h>

void AC_Init(int argc, char *argv[])
{
	MPI_Init(&argc,&argv);
}


void AC_Bsend(void* sendBuf,int destActorId)
{
	int ret = MPI_Bsend( sendBuf, 1, AC_msgDataType, destActorId, 0, MPI_COMM_WORLD );
	if(ret != MPI_SUCCESS ) errorMessage("MPI_Bsend failed");
}

/**
* Internal fuction that is used only from framework in order to 
* check if there any message in the MPI msg queue.
**/
int AC_Iprobe(int* outstanding)
{
	MPI_Status status;
	MPI_Iprobe(MPI_ANY_SOURCE,0,MPI_COMM_WORLD,outstanding,&status);
	return status.MPI_SOURCE;
}

/**
* Internal fuction that is used only from framework for
* receiving and adding messages to actors queue.
**/
int AC_Recv(void* event)
{
	MPI_Status status;
	MPI_Recv( event, 1, AC_msgDataType, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status );
	return status.MPI_SOURCE;
}

/**
* Sends a message to all the process except the sender.
**/
void AC_Bcast(void* msg,int source_actroId)
{
	int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
	for(int actroId=1; actroId < world_size; actroId++)
	{
		if(source_actroId == actroId) continue;
		AC_Bsend(msg,actroId);
	}
	
}

/**
* Every actor use this function in order to receive and handle the messages from 
* other actors. 
**/
static void AC_ActorCode()
{

	int actorType=-1;
	// Receive a message from the master of the poll that determines the type of actor.
	MPI_Recv(&actorType,1,MPI_INT,MPI_ANY_SOURCE,7,MPI_COMM_WORLD,MPI_STATUS_IGNORE); 	
	assert(actorType != -1);

	// Alocate space for message and message queue.
	msg = malloc(AC_msgSizeInBytes);
	for(int i=0; i < AC_MAX_MSG_QUEUE_SIZE; i++)
	{
		msgQueue[i] = malloc(AC_msgSizeInBytes);
	}

	int actorIdQueue[AC_MAX_MSG_QUEUE_SIZE];
	int msgsInQueueCnt			= 0;
	int terminate 				= 0;
	/**
	* Actors receive messages until there are no outstanding messages inside the queue
	* or queue is full. 
	**/
	do
	{
		int outstanding =0;
		AC_Iprobe(&outstanding);
		if(!outstanding || msgsInQueueCnt == AC_MAX_MSG_QUEUE_SIZE-1)
		{
			/**
			* Calls the appropriate fuction based on the actor type.
			* For example if the actor is a squirrel then it will call the SquirelCode function.
			**/
			terminate = (*AC_functPtrs[actorType])(msgQueue,msgsInQueueCnt,actorIdQueue);
			msgsInQueueCnt = 0;			
		}
		else
		{	
			int sourceActorId = AC_Recv(msg);
			/**
			* Copy message inside the queue in order to continue receive messages
			* from others.
			**/
			memcpy((msgQueue[msgsInQueueCnt]),msg,AC_msgSizeInBytes);
			/**
			* Save the actor ids in an array which passed to the actors allong with 
			* the messages in order to allow them to reply if they want.
			**/
			actorIdQueue[msgsInQueueCnt] = sourceActorId;
			msgsInQueueCnt++;
		}
		
	}while(terminate != 1);

	// Informs master that the actor finish.
	MPI_Send(NULL, 0, MPI_INT, 0, 0, MPI_COMM_WORLD);
	
	int workerStatus = 1;
	while (workerStatus) 
	{
		workerStatus = workerSleep();
		// A worker received wakeup command
		if(workerStatus == 1) 
		{
			// A new actor have been born.
			AC_ActorCode(); 	
		}
	}
	

}

/**
* The master of the pool use this function inorder to 
* 1) wake up the actors
* 2) detect when every one is finish.
**/
void AC_PoolMaster()
{
	int activeWorkers=0;
	MPI_Request initialWorkerRequests[AC_numActors];

	// Pool master wakes up all the workers.
	for(int i=0; i<AC_numActors; i++)
	{
			int workerPid = startWorkerProcess();	
			activeWorkers++;
			MPI_Irecv(NULL, 0, MPI_INT, workerPid, 0, MPI_COMM_WORLD, &initialWorkerRequests[i]);		
	}
	
	int cnt=1;
	// Pool master assigns an actor type to each worker.
	for (int actorType=0; actorType < AC_numOfDiffActorTypes; actorType++)
	{
		for(int j=0; j< AC_diffActorsQuantity[actorType]; j++ )
		{
			
			MPI_Ssend(&actorType,1,MPI_INT,cnt,7,MPI_COMM_WORLD);
			cnt++;
	
		}
	}
	int masterStatus = masterPoll();
	int returnCode;

	// Pool master stay inside this loop untill all workers finish.
	while (masterStatus) {
		masterStatus=masterPoll();
		for (int i=0;i< (AC_numActors);i++) {
			// Checks all outstanding workers that master spawned to see if they have completed
			if (initialWorkerRequests[i] != MPI_REQUEST_NULL) {
				MPI_Test(&initialWorkerRequests[i], &returnCode, MPI_STATUS_IGNORE);
				if (returnCode){
					activeWorkers--;	
				} 

			}
		}
		// If we have no more active workers then quit poll loop which will effectively shut the pool down when  processPoolFinalise is called
		if (activeWorkers==0)
		{
			break;
		}
	}
	

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
	int world_size;
	// Check that the the total quantity of different actors
	// is equal with the total actors.
	for(int i=0; i<numOfDiffActorTypes; i++)
	{
		expected_total_actors += quantityForEachType[i];
	}
	assert(expected_total_actors == totalActors);
	

    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    if( (world_size < totalActors) & (AC_GetActorId() == 0) )
    {
		printf("ACTOR ERROR: The number of requested actors is higher than the number of MPI processes!\n\n\n\n");		
		MPI_Abort(MPI_COMM_WORLD,911);
    }

	AC_numActors = totalActors;
	AC_numOfDiffActorTypes = numOfDiffActorTypes;
	
	/*Allocates array for function pointers and copy pointers to array*/
	AC_functPtrs = malloc(sizeof(func_ptr_foreach_actorType)*numOfDiffActorTypes);	
	memcpy(AC_functPtrs,func_ptr_foreach_actorType,sizeof(func_ptr_foreach_actorType)*numOfDiffActorTypes);

	AC_diffActorsQuantity = malloc(sizeof(int) *numOfDiffActorTypes );
    memcpy(AC_diffActorsQuantity,quantityForEachType,sizeof(int) *numOfDiffActorTypes);    
}
 
void AC_SetActorMsgDataType(int msgFields, AC_Datatype* msgDataTypeForEachField, int* blockLen,AC_Aint* disp)
{
    MPI_Datatype* actorMsgTypes 	= msgDataTypeForEachField;
    int* actorMsgBlockLen 	  		= blockLen;
    MPI_Aint* actoMsgDisp 			= disp;
    	
	MPI_Type_create_struct(msgFields, actorMsgBlockLen, actoMsgDisp, actorMsgTypes, &AC_msgDataType);
    MPI_Type_commit(&AC_msgDataType);

    int bufSize,msgSize;
    MPI_Pack_size( 1, AC_msgDataType, MPI_COMM_WORLD, &msgSize );
    bufSize = AC_MAX_MSG_QUEUE_SIZE *msgSize;
    buf = (void*)malloc( bufSize );
    MPI_Buffer_attach( buf, bufSize );

    AC_msgSizeInBytes =0 ;

    // Calculates the message size.
    for(int i=0; i<msgFields;i++)
    {
    	AC_msgSizeInBytes += sizeof(msgDataTypeForEachField[i]);
    }
    
}

/**
* Deallocate dynamically allocated arrays and finialize MPI.
**/
void AC_Finalize()
{
	for(int i=0; i < AC_MAX_MSG_QUEUE_SIZE; i++)
	{
		free(msgQueue[i]);	
	}
	free(msg);
	free(buf);
	free(AC_functPtrs);
	free(AC_diffActorsQuantity);
	MPI_Type_free(&AC_msgDataType);
	MPI_Finalize();
}

/**
* Actors id is the same as MPI rank.
**/
int AC_GetActorId()
{
	int myRank;
	MPI_Comm_rank(MPI_COMM_WORLD, &myRank);
	return (myRank);
}

/**
* An actor calls this function in order to create a new actor.
**/
void AC_CreateNewActor(int actorType,void* startData)
{
	int workerId = startWorkerProcess();
	AC_Bsend(startData,workerId);	
	MPI_Ssend(&actorType,1,MPI_INT,workerId,7,MPI_COMM_WORLD);
}
	
int AC_GetParentActorId()
{
	return getCommandData();
}
