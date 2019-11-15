#ifndef CACHING
#define CACHING

#define MESSAGE_LEN 49
#define RESPONSE_LEN 8
#define SHA_LEN 32
#define CACHE_SIZE 10000
#define PRIME 7753


typedef struct Node
{
	uint8_t response[RESPONSE_LEN];
	uint8_t hash[MESSAGE_LEN];
	struct Node *next;
} Node;

int cache_hash(uint8_t *hash_arr)
{
	int hash = 0;
	int i;
	for (i = 0; i < SHA_LEN; i++)
	{
		hash += hash_arr[i];
	}
	return PRIME * hash % CACHE_SIZE;
}

void cache_insert(int key, uint8_t hash[MESSAGE_LEN], uint8_t response[RESPONSE_LEN], Node* cache[CACHE_SIZE])
{
	struct Node *newNode = malloc(sizeof(Node));
	if (newNode == NULL)
	{
		return;
	}

	memcpy(newNode->hash, hash, MESSAGE_LEN);
	memcpy(newNode->response, response, RESPONSE_LEN);
	newNode->next = NULL;

	if (cache[key] == NULL)
	{
		cache[key] = newNode;
	}
	else
	{
		Node *findNode = cache[key];
		while (1)
		{
			if (findNode->next == NULL)
			{
				findNode->next = newNode;
				break;
			}
			findNode = findNode->next;
		}
	}
}

uint8_t *cache_search(int key, uint8_t *client, Node* cache[CACHE_SIZE])
{
	if (cache[key] == NULL)
	{
		return NULL;
	}

	Node *findNode = cache[key];
	uint8_t sha_good;

	while (1)
	{
		sha_good = 1;

		if (memcmp(client, findNode->hash, SHA_LEN) != 0)
		{
			sha_good = 0;
		}

		if (sha_good || (findNode->next == NULL))
		{
			break;
		}

		findNode = findNode->next;
	}

	if (sha_good)
		return findNode->response;
	else
		return NULL;
}

#endif
