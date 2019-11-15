#ifndef PRIORITY
#define PRIORITY


#define NUMBER_OF_PRIOS 16
#define MESSAGE_LEN 49

//Some setup for the priority heap
typedef struct request{
	uint8_t hash[MESSAGE_LEN];
	uint8_t priority;
	int sock;
} request;

typedef struct QNode{
	request* key;
	struct QNode* next;
} QNode;

typedef struct Queue{
	QNode *front, *rear;
} Queue;

// Function for creating new requests
void initReq(request* r, uint8_t* hash, int sock){
	memcpy(r->hash, hash, MESSAGE_LEN*sizeof(uint8_t));
	r->sock = sock;
	r->priority = hash[48];
}

//Function for creating new queue nodes
QNode* newNode(request* k){
	QNode* temp = (QNode*)malloc(sizeof(QNode));
	temp->key = k;
	temp->next = NULL;
	return temp;
}

// Function for creating new queue
Queue* createQueue(){
	Queue* q = (Queue*)malloc(sizeof(Queue));
	q->front = q->rear = NULL;
	return q;
}

// Function for adding request to queue
void enQueue(Queue* q, request* k){
	// create new node
	QNode* temp = newNode(k);

	//Find index for queue
	int i = k->priority-1;
	// If queue is empty, then new node is both front and rear
	if (q[i].rear == NULL)
	{
		q[i].front = q[i].rear = temp;
		return;
	}
	//Add new node at the end of queue
	q[i].rear->next = temp;
	q[i].rear = temp;
}

//Function for extracting node from highest prio queue with elements
QNode* deQueue(Queue* q){
	//Find highest prio queue with elements
	int i;
	for (i=NUMBER_OF_PRIOS-1; i>=0; i--){
		if (q[i].front!=NULL) break;
	}
	// If empty 
	if (q[i].front==NULL) printf("queue is empty");

	//Store previous front and move front one ahead
	QNode* temp = q[i].front;
	
	q[i].front = q[i].front->next;

	//If front becomes NULL, then change rear to NULL
	if (q[i].front==NULL) q[i].rear=NULL;

	return temp;
}


#endif
