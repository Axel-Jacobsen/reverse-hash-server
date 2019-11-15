#ifndef PRIORITY
#define PRIORITY


#define NUMBER_OF_PRIOS 16
#define MESSAGE_LEN 49

//Some setup for the priority heap
struct request{
	uint8_t package[MESSAGE_LEN];
	uint8_t priority;
	int socket;
};

struct QNode{
	struct request key;
	struct QNode* next;
};

struct Queue{
	struct QNode *front, *rear;
};

// Function for creating new requests
void initReq(struct request* r, uint8_t* package, int sock){
	memcpy(r->package, package, MESSAGE_LEN*sizeof(uint8_t));
	r->socket = sock;
	r->priority = package[48];
}

//Function for creating new queue nodes
struct QNode* newNode(struct request k){
	struct QNode* temp = (struct QNode*)malloc(sizeof(struct QNode));
	temp->key = k;
	temp->next = NULL;
	return temp;
}

// Function for creating new queue
struct Queue* createQueue(){
	struct Queue* q = (struct Queue*)malloc(sizeof(struct Queue));
	q->front = q->rear = NULL;
	return q;
}

// Function for adding request to queue
void enQueue(struct Queue* q, struct request k){
	// create new node
	struct QNode* temp = newNode(k);

	//Find index for queue
	int i = k.priority-1;
	// If queue is empty, then new node is both front and rear
	if (q[i].rear == NULL){
		q[i].front = q[i].rear = temp;
		return;
	}
	//Add new node at the end of queue
	q[i].rear->next = temp;
	q[i].rear = temp;
}

//Function for extracting node from highest prio queue with elements
struct QNode* deQueue(struct Queue* q){
	//Find highest prio queue with elements
	int i;
	for (i=NUMBER_OF_PRIOS-1; i>=0; i--){
		if (q[i].front!=NULL) {break;}
	}
	// If empty 
	if (q[i].front==NULL) printf("queue is empty");

	//Store previous front and move front one ahead
	struct QNode* temp = q[i].front;
	
	q[i].front = q[i].front->next;

	//If front becomes NULL, then change rear to NULL
	if (q[i].front==NULL) q[i].rear=NULL;

	return temp;
}


#endif
