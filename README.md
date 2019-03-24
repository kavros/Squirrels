## Build & Run
* Build project using: ```make```
* Run at the frontend of cirrus using 220 processes and default values of the simulation: ```make run```
* Run at the backend of cirrus: ```make qsub```

## Testing
* Run tests using: ```make validation```

## Run Description
* We can run the project in 2 ways:
  * **without command line arguments** using :```mpirun -n numberOfprocesses ./build/squirrels2 ```
  * **with command line arguments** using: ```mpirun -n numberOfprocesses ./build/squirrels2 -l max_num_of_squirrels -s total_squirrels -i total_infected_squirrels ``` 
* The following table explains the supported flags.

 Argument | Description
 ---      | ---
 -l | determines the maximum allowed number of squirrels. 
 -s | determines the starting number of squirrels.
 -i | determines the starting number of infected squirrels.
 -h | provide a help message.


## References
* I used an external library for command line parsing called [Argtable](https://github.com/argtable/argtable3)
