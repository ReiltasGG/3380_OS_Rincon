# 3360 Operating Systems Programming Assingments

OS Programming Assignments done for Prof Rincon COSC 3360. The idea for these assignment was to implement a Rate Monolithic Scheduling Diagram through various different ways.

## HW1

Problem:

For this assignment, you will create a multithreaded version of the Rate Monotonic Scheduling algorithm (https://www.geeksforgeeks.org/rate-monotonic-scheduling/) implemented on a multiprocessor architecture using a Partitioned Scheduling Approach.

Given an input string representing the periodic tasks executed in a processor, you must show the scheduling information (tasks, hyperperiod, utilization, etc.) and the scheduling diagram (if the task set is schedulable by Rate Monotonic).
For example, given the following string:

A 2 10 B 4 15 C 3 30

A, B, and C represent the tasks executed in the processor,  2 and 10 represent the worst-case execution time and period for task A, 4 and 15 represent the worst-case execution time and period for task B, and 3 and 30 represent the worst-case execution time and period for task A. The task identifier is represented by one character, and the task worst-case execution time and period are represented by a positive integer value.

You can safely assume that for this assignment, the tasks are periodic, have implicit deadlines (deadline = period), and all arrive at time zero. If two or more tasks have the same period, the task priority will be determined by the task identifier lexicographic order (the lower the ASCII value, the higher the priority). 

The hyperperiod of a task set is the least common multiple (LCM) of the tasks' periods. For the previous example, the hyperperiod is the LCM(10,15,30) = 30.

The utilization of a task set is the sum of all WCETi / Period i , for 1 <= i <= number of tasks U=∑i<=nTasksi=1(WCETi/Periodi)
.

A task set is schedulable by Rate Monotonic only if:
U≤n∗(21/n−1)

Where n is the number of tasks in the task set. For this assignment, a task set with utilization greater than one is considered a "Task set not schedulable", and we consider that the "Task set schedulability is unknown" when utilization is greater than n∗(21/n−1)
and less or equal to one. 

Given the previous example, the expected scheduling information is:

Task scheduling information: A (WCET: 2, Period: 10), B (WCET: 4, Period: 15), C (WCET: 3, Period: 30)
Task set utilization: 0.57
Hyperperiod: 30
Rate Monotonic Algorithm execution for CPU1: 
Scheduling Diagram for CPU 1: A(2), B(4), C(3), Idle(1), A(2), Idle(3), 

## HW2


The server program:
The user will execute this program using the following syntax:
./exec_filename port_no
where exec_filename is the name of your executable file and port_no is the port number to create the socket. The
port number will be available to the server program as a command-line argument.
The server program does not receive any information from STDIN and does not print any messages to STDOUT.
The server program executes the following task:
◦ Receive multiple requests from the client program using sockets. Therefore, the server program creates
a child process per request to handle these requests simultaneously. For this reason, the parent process
must handle zombie processes by implementing the fireman() function call (unless you can determine
the number of requests the server program receives from the client program).
Each child process executes the following tasks:
1. First, receive the input with the scheduling information of a CPU from the client program.
2. Next, use the incremental rate monotonic algorithm 
3. Finally, return the calculated values to the client program using sockets.

The client program:
The user will execute this program using the following syntax:
./exec_filename hostname port_no < input_filename
where exec_filename is the name of your executable file, hostname is the address where the server program is
located, port_no is the port number used by the server program, and input_filename is the name of the input file.
The hostname and the port number will be available to the client as command-line arguments.
The client program receives from STDIN (using input redirection) n lines (where n is the number of input strings).
Each line from the input represents the scheduling information of a CPU in a multiprocessor platform

## HW3

For this assignment, you will modify your solution for programming assignment 1 to comply with the restrictions explained below.

Using pthread_join or sleep to synchronize your threads is not allowed (you must use pthread_join to guarantee that the parent thread waits for all its child threads to end before ending its execution). A penalty of 100% will be applied to submissions using the previous system calls to synchronize the child threads. You cannot use different memory addresses to pass the information from the parent thread to the child threads. You must use the output statement format based on the example above.
