# group10

## Final Implementation

The final implementation uses several of the experiments that we used below. At a high level, we used

* Caching
* Priority Ques to process higher priority requests first
* Multithreading consumers for a producer-consumer architecture
* Multithreading for individual reverse hash calculations

The code for caching and the priority queue are in `caching.h` and `priority.h` respectively. The main server implementation is `server.c`.

## Priority

Kasper Bendt Jørgensen (s174293)

Since the weighted scores multiply the run time of a request by the priority of the request, a simple idea for improving the final score, would be to try to lower the run time for higher priority requests, even if that means increasing the run time of lower priority requests.

To do this we decided to insert the request we recieve into a queue of some sort, and when we then want to handle a request we  extract the reqest with the highest priority. Ideally we would like one thread to put request in the queue and other threads to handle the requests, but for this experiment we simply wait untill we have a few request in the queue before we start handling them.

To be able to handle a request we extract from the queue, we need to have a way of getting the socket that the request was sent through. To do this we made a __struct request__ that contains the packet from a request and the socket it came from, and store these structs on the queue.
#### Priority using a single sorted list
To start with we just wanted a simple queue, so we could test the structs, and also see if the priority made a noticable difference, before we started making a more complex queue structure.
For this simple version, we made a single list, and when we insert requests we make sure to insert it at a place in the list that ensures it is sorted. This way we can just take the first element in the list when we want to extract a request.

Using priority implementation with this sorted list we get a score improvement of about 7.6% from the base version.
This is a very small improvement, but it's enough to show that prioritising high-priority request at the cost of low-priority request can make a reasonable improvement, if we make a more efficient queue.

#### Priority using 16 queues
Now we knew the priority implementation couild work, but our queue was very slow. Because of this we decided to make a new experiment with a (on paper) faster queue implementation. 

We decided to make a simple FIFO queue for each level of priority. Since there is 16 priority levels we need 16 queues. For the queues themselves we use linked list queues, since we will have some queues that are empty and some that have a lot of elements. With a linked list we don't have to wory about any of the queues being filled, spending time increasing the size of some of the queues or waisting memory on empty queues. Linked list queues also allow us to have very fast insert and extract operations.

For inserting a request we find the priority of the request and insert it at the end of the queue corosponding to that peiority. For extracting we find the highest priority queue that is not empty, and take the front element in that queue.

Using this implementation for the queue we get a 27.2% score improvement over the base version.
This is a very good improvement, and we would definitely want to utilize this in the final version of the server.

### Test results
All the test-scores from these 2 experiments can be found in the table below. All of the tests for the experiment was run on the same machine using the __run-client-milestone__ test.

| Server-version         | Test 1      | Test 2      | Test 3      | Test 4      | Test 5      | Average score |
|------------------------|-------------|-------------|-------------|-------------|-------------|---------------|
| Base version           | 293,367,363 | 279,708,346 | 274,332,655 | 282,637,657 | 276,751,659 | 285,054,511   |
| 1-sorted-list-priority | 272,559,508 | 276,590,468 | 255,518,303 | 266,730,014 | 254,159,835 | 263,359,672   |
| 16-Queue-priority      | 204,356,849 | 214,204,695 | 211,464,262 | 209,549,930 | 210,652,128 | 207,504,489   |

### Location of the code
The source code for this experiment can be found on the __priority__ branch in the repository, in the server.c file. The most recent commit on the branch contains the version with the 16 FIFO queues. The version with 1 sorted list can be found on an old commit (64b5743) with the commit-message: "working version of simple priority using 1 list".

---------------------------------

# Checking for equality of hashes

## Theory
In the first version of our server we create a hash from the sha256 algorithm for every number between the __start__ and __end__. Then we check if one of those are equal to the original hash that we have received from the packet. We check for equality by traversing all the 32 bytes of both hashes and checking if the are equal. This means we traverse the original hash everytime we need to check for equality with a hash created from the sha256 algorithm. There should be no reason for traversing the original hash everytime we need to check if the correct hash has been found. By converting the 32 bytes of the original hash into four 64-bit integers, we can save them for later use. The sha256 hash also has to be converted to four 64-bit integers. To check for equality we then have to see if the four integers from the original hash are equal to the four integers from the sha256. By doing this we still have to traverse the 32 bytes of the sha256 hash for every sha256 hash that we create. On the other hand we only traverse the original hash once. This should make our server faster since it traverse a lot less data. Furthermore we process a lot less equality checks, instead of doing 32 equality checks per sha256 hash we only do four checks for equality with this alternative way.

## implementation
instead of passing the original hash as an array, the parts such as the __start__, __end__ and __priority__ are selected from the packet and saved as variables in the struct. The original hash is then converted to four 64-bit integers and saved in the struct as variables.

## Test results
All the tests have been run on the same machine and they have been run with the milestone client. The alternative way of equality checking is tested against the base version of the server, they are both tested three times to find an average speed of the different servers.

|             | score 1   | score 2   | score 3   | avg.      |
|-------------|-----------|-----------|-----------|-----------|
| base        | 424811805 | 420360175 | 421381709 | 422184563 |
| alternative | 438156320 | 428630913 | 427886764 | 431557999 |

## Conclusion
The test results show no noticable change in the speed of the server. If anything the server is being slowed down. Our theory is that the compiler is smart enough to see that it has to traverse the same data a lot of times, and therefore it does no difference in the speed of the server when we manually makes it go through less data. Therefore it can be concluded that this alternative way of checking for equality between hashes, does not improve the performance of our server. Therefore it is not implemented in our final solution for the server.

## Where to find code
The code for this experiment can be found on the __alternative_equality_checking__ branch. The __server_with_int.c__ file contains the entire implementation of the alternative way of equality checking.

---------------------------------

## Caching

With a repeatability of 20% in the __run-client-final.sh__, there is a 1/5 chance of the same request coming again, immediately after. Therefor it could be helpful to save the computed hashes, and simply look the hash up, when it is needed, instead of 'guessing' it, from all possible values.
The code for this experiment can be found on the __alternative_equality_checking__ branch. The __server.c__ file is just the base server file, except the original hash is saved in a struct. The __server_with_int.c__ file also contains the alternative way of equality checking.

---------------------------------

## Multithreading - Job delegation

Casper Egholm Jørgensen (s163950) git-user "Cladoc"

As the virtual machine is configured with multiple processor cores it makes sense to conduct experiments with multithreading in an attempt to utilize these capabilities.
I conducted three experiments involving multithreading of which one was included in the final server.

### Popup request handling threads
The first experiment had a main thread continuously listening for requests, and upon accepting one, create a popup-thread to handle the request. The integer identifying the socket with the pending message is passed as parameters when the thread is created. The thread then reads the message from the socket, finds the answer, sends it to the client and terminates.  

### Job delegation - Solution with no shared buffer
Upon initial testing of the popup-threads method, it was hypothesized to contain two issues that could be improved upon. 
First, having an "unlimited number"(comments on this in the discussion) running simultaneously would (occuring for high continuous bursts of requests) lead to congestion on processor usage making the order in which requests arrived indifferent. To achieve lower average delay, the requests would have to be handled in the order they arrive and have those prioritized over later arrivals.
 
Secondly it would be faster if the threads handling requests could be created on server start-up instead of dynamically, avoiding overhead.
In this experiment, instead of creating popup threads for each request, a select number of threads would be created on server start. The main thread would accept incoming requests and delegate it to a worker thread when any is available. If no thread is available, the main thread will wait until one is, identifying each worker thread by their own semaphore.

### Job delegation - Classic producer/consumer scheme
The above solution uses a check and delegate-or-wait solution. If no thread is ready to handle a request, the main thread will sleep for 1 second and check again. However, if the thread is done working early, the main thread is still sleeping, wasting time.
This solution makes use of a classic concurrent programming technique that solves a producer-consumer problem (or bounded-buffer problem) using threads, semaphores and a circular array. Initially, before the main thread launches the server service, it creates a predefined number of idle request handling threads. The main thread is then responsible for listening for established connection on sockets and enqueuing the integer identifying said socket in the queue indicating that a request is available for the worker threads to handle. 

The queue is in this experiment constructed as a FIFO circular array. If a request handling thread is idle/waiting, it will be woken on a job insertion and dequeue a client request socket number from the queue and handle the request.
As both the the main threads and worker threads will be performing enqueues and dequeues on the queue, mutual exclusion is ensured by the use of a mutex protecting against simultaneous buffer modfications from different threads. In addition, "empty" and "full" semaphores are used to allow the main threads and worker threads to communicate on the status of the buffer. Whenever "empty" is 0, the main thread will wait on this semaphore untill signalled by one of the worker threads that an item has been removed from the queue, and a slot therefore available, by incrementing the semaphore. Likewise, worker threads will, if the queue is empty, wait on the "full" semaphore which is incremented by the main thread, when a job is ready in the queue.
This use of semaphores to signal whenever items are ready in the queue and having threads sleep and wake up properly by the nature of semaphores avoids busy-waiting and is thus very efficient. The amount of worker threads idle at any time is easy to modify by changing a single macro "MAX\_THREADS". Good results were found for 4 to 10 threads alive at any time.

### Test results
| Server-version               | Test 1      | Test 2      | Test 3      | Average score |
|------------------------------|-------------|-------------|-------------|---------------|
| Base version                 | 222,218,859 | 222,688,812 | 221,733,401 | 222,213,690   |
| Popup threads                | 148,934,733 | 147,034,193 | 150,031,367 | 148,666,764   |
| Job delegation(4 threads)    | 105,673,304 | 103,258,324 | 102,660,669 | 103,864,099   |
| Producer/consumer(4 threads) | 101,071,113 | 103,732,729 | 103,008,310 | 102,604,050   |

### Discussion
The producer/consumer solution was the technique carried on to the final solution of the three experiments because of its concise implementation, great scalability, ease to integrate with the priority queue experiments and last but not least performance compared with the base implementation. A slight difference is the buffer which is not implemented as a circular array in the final implementation, but as linked lists to conform with the priority queue.
This solution trumps the popup-thread experiment in both performance and safety as it is a well known technique. Performance wise, the sheer amount of popup-threads running concurrently in the first experiment renders the average response for any request very high, as they are all handled concurrently, thus not guaranteeing that request arriving first will be responsed to first. This solution allows for constraints on the maximum number of threads that can operate concurrently while still utilizing the multiple cores. Safety-wise, the popup-thread method cannot create an unlimited number of threads in practice as there is a system limit. The consumer/producer solution avoids this issue. 
This solution was further more chosen over the other delegation technique because of slightly better performance and ease to integrate with the priority queue. 

### Conclusion
The producer/consumer solution was the technique carried on to the final solution of the three experiments because of its concise implementation, great scalability, ease to integrate with the priority queue and last but not least performance compared with the base implementation.

### Location of code
 A branch by the name Multithreaded-Job\_Delegation can be found on the Github repository with 3 directories named "backup\_popup" (First experiment), "backup\_delegation1" (second experiment) and "backup\_consumer\_producer" (final experiment). 

---------------------------------

## Multiprocessing - Job delegation

Axel Jacobsen (s191291)

A simple way to parallelize the task of cracking many hashes simultaneously is with multiprocessing. The purpose of this experiment was to measure the speed of multiprocessing the reverse hash requests with forking, and to eventually compare that to the speed of multithreading.

### Multiprocessing and `waitpid`

The first and simplest implementation is waiting on each process (branch `multiprocessing`, commit `07dcba5384b0c41fef84507bb9e51f2ccdf9bdaa`). The file defines `NUM_FORKS` which is the number of child processes that will be spawned to reverse hash requests.

The main function spins up `NUM_FORKS` processes which each reverses one hash. Immediately after, the parent process runs `waitpid` on each of the child processes sequentially. Once all of the child processes have finished, the parent process creates `NUM_FORKS` new child processes. This is obviously ineficient in time, as the parent proccess has to wait for all of the child processes to finish before starting new child processes. That means that when a child process finishes before its sibling, it dies and then the parent process has one less child performing work.

It would be preferable to have another child proccess be created and to start processing another reverse hash request as soon as a child process dies. This lead me to the next iteration of multiprocessing:

### Multiprocessing and busywaiting

The next implementation of the code used busy waiting to create new child processes immediately after they finish running (branch `multiprocessing`, commit `f36189ab68fc9dc1021f50af4bf6a770e952dd28`). Theoretically, this gives better performance, as there will be more total time of multiprocessing.

This implementation works by initializing an array called `pid` of `NUM_FORK` integers, all set to `-1`. It then checks each element of this array; if the value is `-1`, it forks to process a reverse hash, and sets that element of `pid` to the child processes' pid. After it is done checking and forking, the main process busy-waits on the processes by calling `waitpid(..., WNOHANG)` which returns immediately. If its value is `-1`, then that processes is complete, and the main function loops back to spin up a new child.

An advantage of this method is that the parent process creates new child processes almost immediately after they finish, leading to more reverse hashes in the same amount of time (as compared to the previous implementation). However, it relies on continuously checking the processes, which wastes valuable CPU cycles which could otherwise be used for reverse hashing. The next and final implementation of multiprocessing solves this issue.

### Multiprocessing and signals

The final implementation of the code used signals to notify the parent process of a child finishing it's reverse hash (branch `multiprocessing`, commit `8de2905a4e74bcf3dc45c5e716f8fa26cd21c418`).

A "global fork count" was created and initialized to zero, along with a function called `handle_finished_fork` which decremented the global fork count. The main process created a signal to listen for `SIGHUP`s, and to execute `handle_finished_fork` once it is triggered. The main process creates child processes and increments the global fork count until it reaches `NUM_FORKS`. It then sleeps for one second (although the time that it sleeps could be tuned), and then checks if the global fork count is below `NUM_FORKS`. If it is, it will create new child processes and increment the global fork count until it reaches `NUM_FORKS` again.

The benefit of this implementation is that it uses less busy waiting than the previous implementation. One possible drawback is that it uses a signal `SIGHUP` which could be used by another process, which would lead to undefined behaviour. A drawback to my specific implementation is that it is not threadsafe; the `handle_finished_fork` function should protect the decrement of the global fork count with a semaphore.

Regardless, this implementation was the fastest of all of the previous, and gave the most desirable score. I then changed the `NUM_FORKS` variable and tested the server, which lead to the following scores:

| Number of Forks | Score (Averaged over three runs of `run-client-milestone.sh`) |
|-----------------|---------------------------------------------------------------|
| 4               | 131,827,730                                                   |
| 8               | 132,823,120                                                   |
| 16              | 142,406,391                                                   |
| 32              | 137,639,899                                                   |

### Conclusion

The number of forks that gave the best scores were 4 forks and 8 forks, with not a large difference inbetween them. This is because as the number of forks increases, the CPU uses more cycles switching between each process instead of processing reverse hashes. With this data, we used a smaller number of threads for our final implementation.

---------------------------------

## Multithreading for cracking speed

(s164415) Magnus Lyk-Jensen

On branch: `Magnus Experiment` 

The base implementation iterated from start to end, comparing the SHA conversion with the request. However, it only compared one value at a time, as the reason for this experiment where it will use multithreading to reduce the time for the iteration. 
Instead of going from start to end, multiple threads have each of their section from the base loop. The first experiment have two threads implemented to handle each of their part. This should in theory speed up the “cracking time”, as each thread will be doing one at a time, increasing the speed.  This was done by creating the amount of threads it will be using. Each thread will run a function which takes a struct as argument, in which it will be passed the required values. There is no need for semaphores, as each thread will be using a different section of start-end range than the other thread. When one of the threads finds the solution, it will directly respond instead of returning to the main function.  When this is done, it will close the other threads with signals before being returned. This was done by using `pthread_setcanceltype`, `pthread_testcancel` and `pthread_cancel`. The type would be `DEFERRED` and each of the functions would have a `pthread_testcancel` which would serve as a checkpoint which responds to a cancel request made by `pthread_cancel`. 

First it was tested with 2 thread and then 4 threads. 

## Test results
| Server-version               | Test 1      | Test 2      | Test 3      | Average score |
|------------------------------|-------------|-------------|-------------|---------------|
| Base version                 | 208,381,291 | 192,390,982 | 191,735,671 | 197,502,648   |
| 2 threads                    | 100,641,086 | 102,729,951 | 103,440,548 | 102,270,528   |
| 4 threads                    | 97,263,237  | 100,461,213 | 99,637,479  | 99,120,643    |
