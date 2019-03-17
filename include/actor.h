#ifndef ACTOR_H_
#include "mpi.h"
#define ACTOR_H_
#define AC_EMPTY -1
#define AC_INT 		MPI_INT
#define AC_FLOAT 	MPI_FLOAT
#define AC_CHAR		MPI_CHAR
#define AC_Datatype MPI_Datatype
#define AC_MAX_MSG_QUEUE_SIZE 1000

#define AC_TERMINATE_ACTOR 1
#define AC_KEEP_ACTOR_ALIVE 0

void AC_Init(int argc, char *argv[]);
void AC_Bsend(void* sendBuf,int destActorId);
int AC_Iprobe(int* outstanding);
void AC_Bcast(void* event,int source_actroId);
int  AC_Recv(void* event);
void AC_RunSimulation();
void AC_SetActorTypes(int totalActors,int numOfDiffActorTypes,int* quantityForEachType,int (**func_ptr_foreach_actorType)());
void AC_SetActorMsgDataType(int msgFields, AC_Datatype* msgDataTypeForEachField, int* blockLen,MPI_Aint* disp);
void AC_Finalize();
void  AC_GetStartData(void* startData);
int AC_GetParentActorId();
void AC_CreateNewActor(int actorType,void* startData);
int AC_GetActorId();
#endif /* ACTOR_H_ */

