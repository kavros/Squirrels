# we run our simulation  without infected squirres so we expect that we will not have any infection.
# As a result, we expect that the follwoing command will print 24
mpirun -n 220 ./build/squirrels2  -i 0 | grep "0 infected" | wc -l
 
# the following command runs our simulation with initial number of squirrels 197, maximum number of allowed squirrels 200
# and without infected squirrels. As a result, we expect that the follwing configuration will stop the execution with an error
# that says that we exceed the maximum number of squirrels.
mpirun -n 220 ./build/squirrels2  -l 200 -s 197 -i 0 
