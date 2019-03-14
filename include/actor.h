#ifndef ACTOR_H_
#define ACTOR_H_
#define AC_EMPTY -1
#define AC_INT 		MPI_INT
#define AC_FLOAT 	MPI_FLOAT
#define AC_CHAR		MPI_CHAR
#define AC_BOOL		MPI_C_BOOL
#define AC_Datatype MPI_Datatype

void AC_Init(int argc, char *argv[]);
void actor_Bsend(void* sendBuf,int destActorId);
MPI_Status actor_Receiv(void* event);
static void AC_actorCode();
static void AC_poolMaster();
void AC_runSimulation();
void AC_setActorTypes(int totalActors,int numOfDiffActorTypes,int* quantityForEachType,void (**func_ptr_foreach_actorType)());
void AC_setActorMsgDataType(int msgFields, AC_Datatype* msgDataTypeForEachField, int* blockLen,MPI_Aint* disp);
void AC_finalize();
int AC_getActorId();




#endif /* ACTOR_H_ */

