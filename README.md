# You Spin Me Round Robin

This project implements Round Robin Scheduling that an OS might do for a given workload and quantum
length. The code parses an input file (containing the PID, arrival time, and burst times of all processes) and a command line argument to specify a desired quantum length. The output will display the Average Waiting and Response time that resulted from Round Robin Scheduling.

Assumptions:
1) When quantum length of 0 is passed in, the Average Waiting and Response time are both set to output 0
2) If multiple processes have the same arrival time, they are sorted in increasing order and then enqueued to the ready queue based on PID field
3) When enqueuing processes after a quantum, newly arrived processes are prioritized over the process that just ran


## Building

cmd for building the project
```shell
make
```

## Running

cmd for running
Note that 5 is a placeholder value (Replace with desired quantum value)

```shell
./rr processes.txt 5
```

This is an example of the expected output from the program
```shell
Average waiting time: 7.00
Average response time: 2.75
```

## Cleaning up
cmd for removing binary and object files
```shell
make clean
```
