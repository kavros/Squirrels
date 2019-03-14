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



void AC_actorCode()
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
}

void AC_poolMaster()
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
			//printf("Master started worker with MPI process %d\n",  workerPid);

			/*sendBuf[0] = 1;				//command
			sendBuf[1] = actorType;		//actor type
			sendBuf[2] = AC_EMPTY;		//data*/
			MPI_Ssend(&actorType,1,MPI_INT,workerPid,7,MPI_COMM_WORLD);
		
			
			//actor_Bsend(msg,workerPid);
			cnt++;
	
		}
	}


}


void AC_runSimulation()
{

	int statusCode = processPoolInit();

	if(statusCode == 2) //master
	{

		AC_poolMaster();
	}
	else if(statusCode == 1)
	{
		AC_actorCode();
	}

	processPoolFinalise();	
	MPI_Finalize();

}

void AC_setActorTypes(int totalActors,int numOfDiffActorTypes,int* quantityForEachType,void (**func_ptr_foreach_actorType)())
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
}

void AC_setActorMsgDataType(int msgFields, AC_Datatype* msgDataTypeForEachField, int* blockLen,MPI_Aint* disp)
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


void AC_finalize()
{
	MPI_Finalize();
}

int AC_getActorId()
{
	int myRank;
	MPI_Comm_rank(MPI_COMM_WORLD, &myRank);
	return (myRank);
}