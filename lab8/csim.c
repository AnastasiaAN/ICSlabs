/*	name:Luo Anqi
 *	loginID:515030910431
 */
#include <getopt.h>
#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include "cachelab.h"
typedef struct address
{
	unsigned long long tag;
	unsigned long long set;
	unsigned long long offset;
} address_t;

typedef struct cache_line
{
	int timestamp;
	int valid;
	unsigned long long tag;

} cache_line_t;
typedef cache_line_t* cache_set_t;

typedef struct cache
{
	int s;
	int E;
	int b;
	int time;
	cache_set_t *sets;
} cache_t;

typedef struct simulator
{
	cache_t *cache;
	FILE *file;
	int hit_count;
	int miss_count;
	int eviction_count;
	int v;
} simulator_t;

address_t explain_address(cache_t *cache, unsigned long long addr)
{
	address_t exp;
	int off = cache->s + cache->b;
	exp.offset = addr & ((1 << cache->b) - 1);
	exp.set = (addr & (((1 << cache->s) - 1) << cache->b)) >> cache->b;
	exp.tag = (addr & (~((1 << off) - 1))) >> off;
	//exp.tag = (addr >> off) & (1 >> (sizeof(long long) - off - 1));
	//printf("explain %llx %llx %llx success\n",exp.tag,exp.set,exp.offset);
	return exp;
}

cache_t * init_cache(int s, int E, int b)
{
	int i, j;
	int set_num;
	cache_t * new_cache;

	if (s < 0 || E < 0 || b < 0 || s + b > 64) 
	{
		return NULL;
	}

	set_num = pow(2,s);
	new_cache = (cache_t *)malloc(sizeof(cache_t));
	new_cache->time = 0;
	new_cache->s = s;
	new_cache->E = E;
	new_cache->b = b;
	new_cache->sets = (cache_set_t *)malloc(sizeof(cache_set_t) *set_num);
	for (i = 0; i < set_num; ++i) 
	{
		new_cache->sets[i] = (cache_set_t)malloc(sizeof(cache_line_t) * E);
		for (j = 0; j < E; ++j) 
		{
			new_cache->sets[i][j].valid = 0;
		}
	}
	return new_cache;
}


void access_cache(simulator_t *sim, unsigned long long addr)
{
	cache_t *cache = sim->cache;
	address_t addr_exp = explain_address(cache, addr);
	cache_set_t set = cache->sets[addr_exp.set];
	
	int E = cache->E;
	//printf("cache->E %d\n",E);
	int i;
	int index = -1;
	//int index0 = -1;
	++cache->time;
	for (i = 0; i < E; ++i) 
	{
		if (set[i].valid && (set[i].tag == addr_exp.tag)) 
		{
			/* update timestamp */
			set[i].timestamp = cache->time;
			sim->hit_count++;
			//printf("hit\n");//hit
			return;
		}
	}
	/* if miss */
	//printf("miss\n");
	//printf("%d\n",cache->time);
	int min_valid = cache->time;
	//printf("min_valid:%d\n",min_valid);
	for (i = 0; i < E; i++) 
	{
		//printf("min_valid:%d\n",min_valid);
		if(set[i].timestamp < min_valid)
		{
			//printf("set\n");
			index = i;
			min_valid = set[i].timestamp;
		}
		/*if(set[i].timestamp = 0)
		{
			index0 = i;
		}*/
	}
	//printf("loop done\n");
	if (set[index].valid == 0) 
	{
		sim->miss_count++;//miss
		//printf("miss:%d\n",sim->miss_count);
	} 
	else 
	{
		//printf("miss_eviction\n");
		sim->miss_count++;
		sim->eviction_count++;//eviction
	}
	set[index].timestamp = cache->time;
	set[index].valid = 1;
	set[index].tag = addr_exp.tag;
	//printf("access complete\n");
}


simulator_t* init_simulator(int s, int E, int b, char *filename, int v)
{
	simulator_t * new_simulator = (simulator_t *)malloc(sizeof(simulator_t));
	if (!new_simulator ) 
	{ 
		return NULL;
	}

	new_simulator->cache = init_cache(s, E, b);
	if (!new_simulator->cache) 
	{ 
		free(new_simulator);
		return NULL;
	}

	new_simulator->file = fopen(filename, "r");
	new_simulator->hit_count = 0;
	new_simulator->miss_count = 0;
	new_simulator->eviction_count = 0;
	new_simulator->v = v;
	//printf("init success\n");	
	return new_simulator;
}

void run_simulator(simulator_t *sim)
{
	char op;
	unsigned long long addr;
	int size;

	while(fscanf(sim->file," %c %llx,%d",&op,&addr,&size) != EOF){
		switch(op)
		{
			case 'I':
			{
				continue;
			}
			case 'L':
			{
				//printf("case load\n");
				access_cache(sim,addr);
				break;
			}
			case 'S':
			{
				//printf("case store\n");
				access_cache(sim,addr);
				break;
			}
			case 'M':
			{
				//printf("case modify\n");
				access_cache(sim,addr);
				access_cache(sim,addr);
				break;
			}
		}
		//printf("case done\n\n");
	}
}



int main(int argc, char **argv)
{
    /*int hit_count = 0;
	int miss_count = 0;
	int eviction_count = 0;
	*/
	int opt=-2;
	char *filename = NULL;
	int s = 0;
	int E = 0;
	int b = 0;
	int v = 0;
	while((opt = getopt(argc, argv, "hvs:E:b:t:")) != -1) 
	{
		//printf(" get opt:%c\n",opt);
		switch(opt) 
		{
			case 'v':
				v = 1;
				break;
			case 's':
				s = atoi(optarg);
				//printf("s:%d\n",s);
				break;
			case 'E':
				E = atoi(optarg);
				//printf("E:%d\n",E);
				break;
			case 'b':
				b = atoi(optarg);
				//printf("b:%d\n",b);
				break;
			case 't':
				filename = optarg;
				//printf("filename:%s\n",filename);
				break;
			case 'h':
				printf("Usage: ./%s [-hv] -s <s> -E <E> -b <b> -t <tracefile>\n", argv[0]);
			default:
				return 1;
		}
	}
	simulator_t *sim = init_simulator(s, E, b, filename, v);
	if(sim)
	{
		run_simulator(sim);
		printSummary(sim->hit_count, sim->miss_count, sim->eviction_count);//hit,miss,eviction
	}
	return 0;
}
