#ifndef CACHING
#define CACHING

#define MESSAGE_LEN 49
#define RESPONSE_LEN 8
#define SHA_LEN 32
#define CACHE_SIZE 10000
#define PRIME 7753


typedef struct node
{
        uint64_t response;
        uint8_t hash[MESSAGE_LEN];
        struct node *next;
} node;

node *cache[CACHE_SIZE] = {NULL};

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

void cache_insert(int key, uint8_t hash[MESSAGE_LEN], uint64_t response)
{
        struct node *newNode = malloc(sizeof(node));
        if (newNode == NULL)
        {
                return;
        }

        memcpy(newNode->hash, hash, MESSAGE_LEN);
	newNode->response = response;
        //memcpy(newNode->response, response, RESPONSE_LEN);
        newNode->next = NULL;

        if (cache[key] == NULL)
        {
                cache[key] = newNode;
        }
        else
        {
                node *findNode = cache[key];
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

int64_t cache_search(int key, uint8_t *client)
{
        if (cache[key] == NULL)
        {
                return -1;
        }

        node *findNode = cache[key];
        uint8_t sha_good = 1;
        int i;

        while (1)
        {
                sha_good = 1;

                for (i = 0; i < SHA_LEN; i++)
                {
                        if (client[i] != (findNode->hash)[i])
                        {
                                sha_good = 0;
                                break;
                        }
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
                return -1;
}

#endif
