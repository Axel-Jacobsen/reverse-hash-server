#ifindef CACHING
#define CACHING

#define CACHE_SIZE 10000
#define PRIME 7753

typedef struct node
{
    uint64_t value;
    struct node* next;
}
node;

node* cache[CACHE_SIZE] = {NULL};

int cache_hash(uint8_t* hash_arr)
{
    int i, hash;
		for(i = 0; i < SHA_LEN; i++){
			hash += hash_arr[i];
		}
		return PRIME*hash%CACHE_SIZE;
}

void cache_insert(int key, uint64_t value)
{
    struct node* newNode = malloc(sizeof(node));
    if (newNode == NULL){
        return;
    }

    newNode->value = value;
    newNode->next = NULL;

    if (cache[key] == NULL){
        cache[key] = newNode;
    }
    else{
        node* findNode = cache[key];
        while (1){
            if (findNode->next == NULL){
                findNode->next = newNode;
                break;
            }
            findNode = findNode->next;
        }
    }
}

 int64_t cache_search(int key, uint8_t* client){
    if (cache[key] == NULL){
        return -1;
    }

  node* findNode = cache[key];
  uint8_t sha256_test[SHA_LEN] = {0};
  uint8_t sha_good = 1;
  int i;
  uint64_t pred_value;

  while (1) {
    sha_good = 1;
    sha256(&findNode->value, sha256_test);

    for(i = 0; i < SHA_LEN; i++){
			if(client[i] != sha256_test[i]){
				sha_good = 0;
        break;
			}
		}
    if(sha_good){
      break;
    }
    if(findNode->next == NULL){
      break;
    }

    findNode = findNode->next;
  }
  if(sha_good){
    return (int64_t) findNode->value;
  }
  else{
    return -1;
  }
}
#endif
