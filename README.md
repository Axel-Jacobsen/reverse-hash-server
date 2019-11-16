# group10

## Priority
Since the weigtet scores multiply the run time of a request by the priority of the request, a simple idea for improving the final score, would be to try to lower the run time for higher priority requests, even if that means increasing the run time of lower priority requests.
To do this we have decided to insert the request we recieve into a queue of some sort, and then once we have a certain amount of request in this queue we extract the request with the highest priority from the queue and handle that request. Once we start working with multible threads we can think about having one thread for putting request into the queue and a number of threads handling requests from the queue. This way we wouldn't need to wait untill we have a certain amount of requests in the queue and can just start handling them immedeatly.
To be able to handle a request we extract from the queue, we need to have a way of getting the socket that the request was sent through. To do this we make a __struct request__ that contains the packet from a request and the socket it came from, and store these structs on the queue.
Now we have most of the stuff needed to implement a priority queue, 
#### Priority using a single list
Using a single sorted list to store requests

#### Priority using 16 queues
Using a first in first out queue for every level of priority. (fastest)

Server-version | Test 1 | Test 2 | Test 3 | Test 4 | Test 5 | Average score
---------------|--------|--------|--------|--------|--------|--------------
Base version | 293,367,363 | 279,708,346 | 274,332,655 | 282,637,657 | 276,751,659 | 285,054,511
1-sorted-list-priority | 272,559,508 | 276,590,468 | 255,518,303 | 266,730,014 | 254,159,835 | 263,359,672
16-Queue-priority |204,356,849 | 214,204,695 | 211,464,262 | 209,549,930 | 210,652,128 | 207,504,489




# Checking for equality of hashes

## Theory
In the first version of our server we create a hash from the sha256 algorithm for every number between the __start__ and __end__. Then we check if one of those are equal to the original hash that we have received from the packet. We check for equality by traversing all the 32 bytes of both hashes and checking if the are equal. This means we traverse the original hash everytime we need to check for equality with a hash created from the sha256 algorithm. There should be no reason for traversing the original hash everytime we need to check if the correct hash has been found. By converting the 32 bytes of the original hash into four 64-bit integers, we can save them for later use. The sha256 hash also has to be converted to four 64-bit integers. To check for equality we then have to see if the four integers from the original hash are equal to the four integers from the sha256. By doing this we still have to traverse the 32 bytes of the sha256 hash for every sha256 hash that we create. On the other hand we only traverse the original hash once. This should make our server faster since it traverse a lot less data. 

## Implementation
Instead of passing the original hash as an array, the parts such as the __start__, __end__ and __priority__ are selected from the packet and saved as variables in the struct. The original hash is then converted to four 64-bit integers and saved in the struct as variables.

## Test results
All the tests have been run on the same machine.        
## Conclusion
The test results show no noticable change in the speed of the server. Therefore it can be concluded that this alternative way of checking for equality between hashes, does not improve the performance of our server. Therefore it is not implemen

## Where to find code

## Caching
With a repeatability of 20% in the __run-client-final.sh__, there is a 1/5 chance of the same request coming again, immediately after. Therefor it could be helpful to save the computed hashes, and simply look the hash up, when it is needed, instead of 'guessing' it, from all possible values.
The code for this experiment can be found on the __alternative_equality_checking__ branch. The __server.c__ file is just the base server file, except the original hash is saved in a struct. The __server_with_int.c__ file also contains the alternative way of equality checking.
