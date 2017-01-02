#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef char* string;
typedef unsigned int uint;

typedef struct cache_block
{
	int tag;
	int lastUsed;
} CacheBlock;

typedef struct arguments
{
	uint print_help,verbose,set_bits,assoc,block_bits;
	string file_name;

} Arguments;

const char TYPE_LOAD = 'L';
const char TYPE_STORE = 'S';
const char TYPE_MODIFY = 'M';
const int EVICTING = 2;
const int HIT = 1; 

typedef CacheBlock* CacheSet;
typedef CacheSet* CacheMem;

typedef struct struct_cache
{
	uint tag_mask,index_mask,assoc, block_bits;
	CacheMem cache_mem;
} Cache;

typedef char InstructionType;

void getParameters(Arguments* args, int argc, string argv[]);
void init(Cache* cache, Arguments* args, int argc, string argv[]);
string processNextInstruction(Cache* cache, InstructionType type, uint address,
							 uint time_step,int* hits, int* misses, int* evictions);
uint getIndex(Cache* cache, uint address);
int getTag(Cache* cache, uint address);
uint getOffset(Cache* cache, uint address);

int main(int argc, string argv[])
{
	Arguments args;
	Cache cache;
	init(&cache,&args, argc, argv);
	FILE* tracefile = fopen(args.file_name,"r");
	InstructionType type;
	uint address;
	uint size;
	string toPrint;
	int misses = 0;
	int hits = 0;
	int evictions = 0;
	int time_step = 1;
	
	while(fscanf(tracefile," %c %x,%i\n", &type,&address,&size) != EOF)
	{
		toPrint = processNextInstruction(&cache,type,address,time_step++,
										&hits,&misses,&evictions);
		if(toPrint)
		{
			if(args.verbose)
			{
				printf("%c %x,%i %s\n", type,address,size,toPrint);
			}
			free(toPrint);
		}
	}
    
    printf("hits:%d misses:%d evictions:%d\n", hits, misses, evictions);
    
    return 0;
}
void init(Cache* cache, Arguments* args, int argc, string argv[])
{
	getParameters(args, argc, argv);
	uint len = (1U << args->set_bits);
	cache->cache_mem = (CacheMem) malloc(sizeof(CacheSet*) * len);
	cache->index_mask = ((1U << args->set_bits)-1U) << args->block_bits;
	//printf("index_mask == %x\n",cache->index_mask);
	uint offset_mask = (1U << args->block_bits)-1;
	cache->tag_mask = ~(cache->index_mask | offset_mask);
	cache->block_bits = args->block_bits;
	cache->assoc = args->assoc;
	for (int i = 0; i < len; i++)
	{
		cache->cache_mem[i] = malloc(sizeof(CacheBlock) * cache->assoc);
		for (int j = 0; j < cache->assoc; j++)
		{
			cache->cache_mem[i][j].tag = -1;
			cache->cache_mem[i][j].lastUsed = 0;	
		}
	}
}
void getParameters(Arguments* args, int argc, string argv[])
{
	args->verbose = 0;
	for(int i = 0; i < argc; i++)
	{
		if(strcmp(argv[i], "-t") == 0)
		{
			args->file_name = argv[++i];
		}
		else if(strcmp(argv[i], "-b") == 0)
		{
			args->block_bits = atoi(argv[++i]);
		}
		else if(strcmp(argv[i], "-s") == 0)
		{
			args->set_bits = atoi(argv[++i]);
		}
		else if(strcmp(argv[i], "-E") == 0)
		{
			args->assoc = atoi(argv[++i]);
		}
		else if(strcmp(argv[i], "-v") == 0)
		{
			args->verbose = 1;
		}
	}
}


string processNextInstruction(Cache* cache, InstructionType type, uint address, 
							uint time_step, int* hits, int* misses, int* evictions)
{
	if (type == TYPE_LOAD || type == TYPE_STORE)
	{
		string toRet = malloc(sizeof(char) * 80);
		uint index = getIndex(cache,address);
		int tag = getTag(cache,address);
		int result = 0;
		int idx = 0;
		int lastUsed = 1 << 30;
		CacheBlock cache_block;
		
		for (int i = 0; i < cache->assoc; i++)
		{
			cache_block = cache->cache_mem[index][i]; 
			if(cache_block.tag == tag) 
			{
				result = HIT;
				idx = i;
				break;
			}
			else if (cache_block.lastUsed < lastUsed )
			{
				lastUsed = cache_block.lastUsed;
				idx = i;
				if(cache_block.tag == -1) result = 0;
				else result = EVICTING;
			}
		}
		
		cache->cache_mem[index][idx].tag = tag;
		cache->cache_mem[index][idx].lastUsed = time_step;
		
		if (result == HIT) 
		{
			strcpy(toRet,"hit");
			(*hits)++;
		}
		else if(result == EVICTING) 
		{
			strcpy(toRet,"miss eviction");
			(*evictions)++;
			(*misses)++;
		}
		else 
		{
			strcpy(toRet,"miss");
			(*misses)++;
		}

		return toRet;
	}
	else if (type == TYPE_MODIFY)
	{
		string tmp = processNextInstruction(cache, TYPE_LOAD, address,
											 time_step,hits,misses,evictions);
		string tmp2 = processNextInstruction(cache, TYPE_STORE, address,
											 time_step,hits,misses,evictions);
		strcat(tmp, " ");
		strncat(tmp,tmp2,20);
		free(tmp2);
		return tmp;
	}
	else return NULL;
}
uint getIndex(Cache* cache, uint address)
{
	return (address & cache->index_mask) >> (cache->block_bits);
}
int getTag(Cache* cache, uint address)
{
	return address & cache->tag_mask;
}