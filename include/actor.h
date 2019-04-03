#ifndef ACTOR_H_
#include "mpi.h"
#include "pool.h"
#define ACTOR_H_
#define AC_EMPTY -1
#define AC_INT 		MPI_INT
#define AC_FLOAT 	MPI_FLOAT
#define AC_CHAR		MPI_CHAR
#define AC_Datatype MPI_Datatype
#define AC_MAX_MSG_QUEUE_SIZE 1000
#define AC_Aint MPI_Aint

#define AC_TERMINATE_ACTOR 1
#define AC_KEEP_ACTOR_ALIVE 0


int AC_numActors; 			// Holds the total number of actors.
int AC_msgSizeInBytes;		// Holds the message size.
int AC_numOfDiffActorTypes;	// Holds the number of different actors.
int* AC_diffActorsQuantity;	// An array that includes how many actors we have for each type.
int (**AC_functPtrs)();		// Holds a fuction pointer for each type of actor.
MPI_Datatype AC_msgDataType;	// Holds the MPI datatype for the message that actor use.

void* msgQueue[AC_MAX_MSG_QUEUE_SIZE];	// the array/queue that holds all actors messages
void* msg; 								// every actor use this buffer to store a message before copy it to the queue
void* buf; 								// we use this buffer to initialize MPI_Attach function.

/**
* Every actor needs to call this function before every other 
* fuction inside the framework.
**/
void AC_Init(int argc, char *argv[]);

/**
* Actors can call this fuction in order to commuincate with other actors.
**/
void AC_Bsend(void* sendBuf,int destActorId);

/**
* Actors can call this method in order to send a meesage 
* to all the other actors.
**/
void AC_Bcast(void* msg,int source_actroId);

/**
* This is the main function of the simulation which every actor calls.
* Based on the status code that the process poll returns we decide 
* if the actor is the master of the pool or worker and then we call
* the appropriate fuction.
**/
void AC_RunSimulation();

/**
* Every actor needs to call this fuction inorder to pass to the framework all the values
* necessary values such as number of actors, type of actors, quantity of each type and function pointers.
**/
void AC_SetActorTypes(int totalActors,int numOfDiffActorTypes,int* quantityForEachType,int (**func_ptr_foreach_actorType)());

/**
* Every actor needs to call this fuction inorder to pass to the framework all the values
* necessary values for the initialization of the MPI datatype that actors using for sending their messages.
**/
void AC_SetActorMsgDataType(int msgFields, AC_Datatype* msgDataTypeForEachField, int* blockLen,MPI_Aint* disp);

/**
* Every actor needs to call this function at the end of the exection.
**/
void AC_Finalize();

/**
* Returns the actor id of the parent process.
**/
int  AC_GetParentActorId();

/**
* An actor call this function whenever wants to crete a new actor.
* Using this function a parent  actor is able to:
*	1) select the type of child actor
*	2) send start data to the child actor.
**/
void AC_CreateNewActor(int actorType,void* startData);

/**
* Returns a unique identifier for each actor.
**/
int  AC_GetActorId();
#endif /* ACTOR_H_ */

