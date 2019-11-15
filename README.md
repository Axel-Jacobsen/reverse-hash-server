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
