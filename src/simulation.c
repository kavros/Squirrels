#include "simulation.h"
#include "argtable3.h"

int getGlobalClockActorId()
{
	return NUM_OF_SQUIRRELS+NUM_OF_CELLS+1;
}

/**
* Returns the actor id based on the cell number.
**/
int getActorIdFromCell(int cellNum)
{
	int cellNumStart = NUM_OF_SQUIRRELS+1;
	return cellNumStart+cellNum;
}

/**
* Returns the cell number based on actor id.
**/
int getCellNumFromActorId(int actorId)
{
	return actorId - NUM_OF_SQUIRRELS -1;
}

int initCmdLineArgs(int argc, char *argv[],int actorId)
{
	struct arg_lit *help;
	struct arg_int *limit,*squirrels,*infection;
	struct arg_end *end;

	void *argtable[] = {
	    help    = arg_litn(NULL, "help", 0, 1, "display this help and exit"),
	    limit   = arg_intn("l", "level", "<n>", 0, 1, "MAX_NUM_OF_SQUIRRELS"),
	    squirrels   = arg_intn("s", "squirrels", "<n>", 0, 1, "NUM_OF_SQUIRRELS"),
	    infection   = arg_intn("i", "infection", "<n>", 0, 1, "INITIAL_NUM_OF_INFECTED_SQUIRRELS"),
	    end     = arg_end(20),
	};

	int exitcode = 0;
    char progname[] = "squirrels2";
    
    int nerrors;
    nerrors = arg_parse(argc,argv,argtable);

    /* special case: '--help' takes precedence over error reporting */
    if (help->count > 0)
    {
    	if(actorId == 0)
    	{
	        printf("Usage: %s", progname);
	        arg_print_syntax(stdout, argtable, "\n");
	        printf("Demonstrate command-line parsing in argtable3.\n\n");
	        arg_print_glossary(stdout, argtable, "  %-25s %s\n");
    	}
        exitcode = 1;
        goto exit;
    }

    /* If the parser returned any errors then display them and exit */
    if (nerrors > 0)
    {
        /* Display the error details contained in the arg_end struct.*/
        if(actorId == 0)
        {
        	arg_print_errors(stdout, end, progname);
        	printf("Try '%s --help' for more information.\n", progname);
        }
        exitcode = 1;

        goto exit;
    }

    if(limit->count > 0)
    {
		 MAX_NUM_OF_SQUIRRELS= limit->ival[0];
    }
    if(squirrels->count > 0)
    {
    	NUM_OF_SQUIRRELS = squirrels->ival[0];
    }
    if(infection->count > 0)
    {
    	INITIAL_NUM_OF_INFECTED_SQUIRRELS = infection->ival[0];
    }


	if(MAX_NUM_OF_SQUIRRELS <= NUM_OF_SQUIRRELS)
	{
		if(actorId == 0) printf("[ERROR]  MAX_NUM_OF_SQUIRRELS <= NUM_OF_SQUIRRELS \n");
		exitcode =1;
		goto exit;
	}


exit:
    /* deallocate each non-null entry in argtable[] */
    arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));

    return exitcode;

}
