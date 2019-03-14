#ifndef ACTOR_H_
#include "mpi.h"
#define ACTOR_H_
#define AC_EMPTY -1
#define AC_INT 		MPI_INT
#define AC_FLOAT 	MPI_FLOAT
#define AC_CHAR		MPI_CHAR
#define AC_BOOL		MPI_C_BOOL
#define AC_Datatype MPI_Datatype
#define AC_MAX_MSG_QUEUE_SIZE 1000

void AC_Init(int argc, char *argv[]);
void AC_Bsend(void* sendBuf,int destActorId);
int AC_Iprobe(int* outstanding);
void AC_Bcast(void* event,int source_actroId);
int  AC_Recv(void* event);
void AC_RunSimulation();
void AC_SetActorTypes(int totalActors,int numOfDiffActorTypes,int* quantityForEachType,void (**func_ptr_foreach_actorType)());
void AC_SetActorMsgDataType(int msgFields, AC_Datatype* msgDataTypeForEachField, int* blockLen,MPI_Aint* disp);
void AC_Finalize();
int AC_GetActorId();

int AC_GetMSgSize();





#endif /* ACTOR_H_ */

