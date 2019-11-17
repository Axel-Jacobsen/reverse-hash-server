# group10

## Priority

Since the weighted scores multiply the run time of a request by the priority of the request, a simple idea for improving the final score, would be to try to lower the run time for higher priority requests, even if that means increasing the run time of lower priority requests.
To do this we have decided to insert the request we recieve into a queue of some sort, and then once we have a certain amount of request in this queue we extract the request with the highest priority from the queue and handle that request. Once we start working with multible threads we can think about having one thread for putting request into the queue and a number of threads handling requests from the queue. This way we wouldn't need to wait untill we have a certain amount of requests in the queue and can just start handling them immedeatly.
To be able to handle a request we extract from the queue, we need to have a way of getting the socket that the request was sent through. To do this we make a __struct request__ that contains the packet from a request and the socket it came from, and store these structs on the queue.
Now we have most of the stuff needed to implement a priority queue,

### Priority using a single list

Using a single sorted list to store requests

### Priority using 16 queues

Using a first in first out queue for every level of priority. (fastest)

| Server-version         | Test 1      | Test 2      | Test 3      | Test 4      | Test 5      | Average score |
|------------------------|-------------|-------------|-------------|-------------|-------------|---------------|
| Base version           | 293,367,363 | 279,708,346 | 274,332,655 | 282,637,657 | 276,751,659 | 285,054,511   |
| 1-sorted-list-priority | 272,559,508 | 276,590,468 | 255,518,303 | 266,730,014 | 254,159,835 | 263,359,672   |
| 16-Queue-priority      | 204,356,849 | 214,204,695 | 211,464,262 | 209,549,930 | 210,652,128 | 207,504,489   |

---------------------------------

## Checking for equality of hashes

### Theory

In the first version of our server we create a hash from the sha256 algorithm for every number between the __start__ and __end__. Then we check if one of those are equal to the original hash that we have received from the packet. We check for equality by traversing all the 32 bytes of both hashes and checking if the are equal. This means we traverse the original hash everytime we need to check for equality with a hash created from the sha256 algorithm. There should be no reason for traversing the original hash everytime we need to check if the correct hash has been found. By converting the 32 bytes of the original hash into four 64-bit integers, we can save them for later use. The sha256 hash also has to be converted to four 64-bit integers. To check for equality we then have to see if the four integers from the original hash are equal to the four integers from the sha256. By doing this we still have to traverse the 32 bytes of the sha256 hash for every sha256 hash that we create. On the other hand we only traverse the original hash once. This should make our server faster since it traverse a lot less data.

### Implementation

Instead of passing the original hash as an array, the parts such as the __start__, __end__ and __priority__ are selected from the packet and saved as variables in the struct. The original hash is then converted to four 64-bit integers and saved in the struct as variables.

### Test results

All the tests have been run on the same machine.

### Conclusion

The test results show no noticable change in the speed of the server. Therefore it can be concluded that this alternative way of checking for equality between hashes, does not improve the performance of our server. Therefore it is not implemen

### Where to find code

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

### Job delegation - Solution with no shared buffer

This solution was

### Job delegation - Classic producer/consumer scheme

This solution was the technique carried on to the final solution of the three experiments because of its clear and concise implementation, great scalability, ease to integrate with the priority queue and last but not least performance.
This solution makes use of a classic concurrent programming technique that solves a producer-consumer problem (or bounded-buffer problem) using threads, semaphores and a circular array. Initially, before the main thread launches the server service, it creates a predefined number of idle request handling threads. The main thread is then responsible for listening for established connection on sockets and enqueuing the integer identifying said socket in the queue indicating that a request is available for the worker threads to handle.
The queue is in this experiment constructed as a FIFO circular array. If a request handling thread is idle/waiting, it will be woken on a job insertion and dequeue a client request socket number from the queue and handle the request.
As both the the main threads and worker threads will be performing enqueues and dequeues on the queue, mutual exclusion is ensured by the use of a mutex protecting against simultaneous buffer modfications from different threads. In addition, "empty" and "full" semaphores are used to allow the main threads and worker threads to communicate on the status of the buffer. Whenever "empty" is 0, the main thread will wait on this semaphore untill signalled by one of the worker threads that an item has been removed from the queue, and a slot therefore available, by incrementing the semaphore. Likewise, worker threads will, if the queue is empty, wait on the "full" semaphore which is incremented by the main thread, when a job is ready in the queue.
This use of semaphores to signal whenever items are ready in the queue and having threads sleep and wake up properly by the nature of semaphores avoids busy-waiting and is thus very efficient. The amount of worker threads idle at any time is easy to modify by changing a single macro. Good results were found for 4 to 10 threads alive at any time.
This solution trumps the popup-thread experiment in both performance and safety as it is a well known technique. Performance wise, the sheer amount of popup-threads running concurrently in the first experiment renders the average response for any request very high, as they are all handled concurrently, thus not guaranteeing that request arriving first will be responsed to first. This solution allows for constraints on the maximum number of threads that can operate concurrently while still utilizing the multiple cores.
This solution was further more chosen over the other delegation technique because of slightly better performance, easy scalability and ease to integrate with the priority queue.

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
| 4               | 131827730                                                     |
| 8               | 132823120                                                     |
| 16              | 142406391                                                     |
| 32              | 137639899                                                     |

The number of forks that gave the best scores were 4 forks and 8 forks, with not a large difference inbetween them. This is because as the number of forks increases, the CPU uses more cycles switching between each process instead of processing reverse hashes. With this data, we used a smaller number of threads for our final implementation.
