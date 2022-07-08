#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "cache.h"
#include "main.h"

/* cache configuration parameters */
static int cache_usize = DEFAULT_CACHE_SIZE;
static int cache_block_size = DEFAULT_CACHE_BLOCK_SIZE;
static int words_per_block = DEFAULT_CACHE_BLOCK_SIZE / WORD_SIZE;
static int cache_writeback = DEFAULT_CACHE_WRITEBACK;
static int cache_writealloc = DEFAULT_CACHE_WRITEALLOC;
static int num_core = DEFAULT_NUM_CORE;

/* cache model data structures */
/* max of 8 cores */
static cache moesi_cache[8];
static cache_stat moesi_cache_stat[8];


/*********** Utilities ***********/
void print_cache(unsigned pid)
{
    cache_line *current = 0;

    current = moesi_cache[pid].LRU_head;
    while (current)
    {
        printf("tag: %u, state: %d\n", current->tag, current->state);
        current = current->LRU_next;
    }
}

void print_caches()
{
    int i;
    for (i = 0; i < num_core; i++)
    {
        printf("cache %d -----------------\n", i);
        print_cache(i);
    }
}

unsigned round_up(unsigned num, unsigned base)
{
    return (num / base) * base + (num % base != 0 ? base : 0);
}

unsigned round_down(unsigned num, unsigned base)
{
    return (num / base) * base;
}

unsigned calculate_tag(unsigned address)
{
    return round_down(address, cache_block_size);
}
/************************************************************/

/************************************************************/
void set_cache_param(param, value) int param;
int value;
{
    switch (param)
    {
    case NUM_CORE:
        num_core = value;
        break;
    case CACHE_PARAM_BLOCK_SIZE:
        cache_block_size = value;
        words_per_block = value / WORD_SIZE;
        break;
    case CACHE_PARAM_USIZE:
        cache_usize = value;
        break;
    default:
        printf("error set_cache_param: bad parameter value\n");
        exit(-1);
    }
}
/************************************************************/

/************************************************************/
void init_cache()
{
    int i;

    /* initialize the cache, and cache statistics data structures */
    for (i = 0; i < num_core; i++)
    {
        moesi_cache[i].LRU_head = NULL;
        moesi_cache[i].LRU_tail = NULL;
        moesi_cache[i].id = i;
        moesi_cache[i].size = DEFAULT_CACHE_SIZE;
        moesi_cache[i].cache_contents = 0; 
    }
}
/************************************************************/

/************************************************************/
void perform_access(unsigned int addr, unsigned int access_type, unsigned int pid)
{
    int i, idx;
    Pcache c;

    /* handle accesses to the moesi caches */
    switch (access_type)
    {
    case TRACE_LOAD:
        perform_access_load(addr, pid);
        break;
    case TRACE_STORE:
        perform_access_store(addr, pid);
        break;
    }
}
/************************************************************/

/************************************************************/
void flush()
{
    for (int i = 0; i < num_core; i ++)
    {
        cache_line *head = moesi_cache[i].LRU_head;
        while (head)
        {
            if (head->state == STATE_MODIFIED || head->state == STATE_OWNED)
            {
                moesi_cache_stat[i].copies_back ++;
            }
            delete(&moesi_cache[i].LRU_head, &moesi_cache[i].LRU_tail, head);
            head = moesi_cache[i].LRU_head;
        }
    }
}
/************************************************************/

/************************************************************/
void delete (head, tail, item)
    Pcache_line *head,
    *tail;
Pcache_line item;
{
    if (item->LRU_prev)
    {
        item->LRU_prev->LRU_next = item->LRU_next;
    }
    else
    {
        /* item at head */
        *head = item->LRU_next;
    }

    if (item->LRU_next)
    {
        item->LRU_next->LRU_prev = item->LRU_prev;
    }
    else
    {
        /* item at tail */
        *tail = item->LRU_prev;
    }
}
/************************************************************/

/************************************************************/
/* inserts at the head of the list */
void insert(head, tail, item)
    Pcache_line *head,
    *tail;
Pcache_line item;
{
    item->LRU_next = *head;
    item->LRU_prev = (Pcache_line)NULL;

    if (item->LRU_next)
        item->LRU_next->LRU_prev = item;
    else
        *tail = item;

    *head = item;
}
/************************************************************/

/************************************************************/
void dump_settings()
{
    printf("Cache Settings:\n");
    printf("\tSize: \t%d\n", cache_usize);
    printf("\tBlock size: \t%d\n", cache_block_size);
}
/************************************************************/

/************************************************************/
void print_stats()
{
    int i;
    int demand_fetches = 0;
    int copies_back = 0;
    int broadcasts = 0;
    int reply_fetches = 0;

    printf("*** CACHE STATISTICS ***\n");

    for (i = 0; i < num_core; i++)
    {
        printf("  CORE %d\n", i);
        printf("  accesses:        %d\n", moesi_cache_stat[i].accesses);
        printf("  misses:          %d\n", moesi_cache_stat[i].misses);
        printf("  miss rate:       %f (%f)\n",
               (float)moesi_cache_stat[i].misses / (float)moesi_cache_stat[i].accesses,
               1.0 - (float)moesi_cache_stat[i].misses / (float)moesi_cache_stat[i].accesses);
        printf("  evictions:       %d\n", moesi_cache_stat[i].evictions);
        printf("  num fetches:     %d\n", moesi_cache_stat[i].demand_fetches);
        printf("  num responses:   %d\n", moesi_cache_stat[i].reply_fetches);
        printf("\n");
    }

    printf("  TRAFFIC\n");
    for (i = 0; i < num_core; i++)
    {
        demand_fetches += moesi_cache_stat[i].demand_fetches;
        copies_back += moesi_cache_stat[i].copies_back;
        broadcasts += moesi_cache_stat[i].broadcasts;
        reply_fetches += moesi_cache_stat[i].reply_fetches;
    }
    printf("  demand fetch (words):           %d\n", (demand_fetches)*cache_block_size / WORD_SIZE);
    printf("  broadcasts:                     %d\n", broadcasts);
    printf("  cache-to-cache copies (words):  %d\n", (reply_fetches)*cache_block_size / WORD_SIZE);
    printf("  copies back to memory (words):  %d\n", (copies_back)*cache_block_size / WORD_SIZE);

    printf("\n*** VALIDATION REPORT ***\n");
    printf("[");
    for (i = 0; i < num_core; i++)
    {
        printf("%d", moesi_cache_stat[i].accesses);
        if (i + 1 != num_core)
        {
            printf(", ");
        }
    }
    printf("]\n[");
    for (i = 0; i < num_core; i++)
    {
        printf("%d", moesi_cache_stat[i].misses);
        if (i + 1 != num_core)
        {
            printf(", ");
        }
    }
    printf("]\n");
    printf("%d\n", (demand_fetches)*cache_block_size / WORD_SIZE);
    printf("%d\n", broadcasts);
    printf("%d\n", (reply_fetches)*cache_block_size / WORD_SIZE);
    printf("%d\n", (copies_back)*cache_block_size / WORD_SIZE);
}
/************************************************************/

/************************************************************/
void init_stat(Pcache_stat stat)
{
    stat->accesses = 0;
    stat->misses = 0;
    stat->evictions = 0;
    stat->demand_fetches = 0;
    stat->reply_fetches = 0;
    stat->copies_back = 0;
    stat->broadcasts = 0;
}

/************************************************************/

void move_to_front(int i, cache_line *line)
{
     delete(&moesi_cache[i].LRU_head, &moesi_cache[i].LRU_tail, line);
    insert(&moesi_cache[i].LRU_head, &moesi_cache[i].LRU_tail, line);
}

cache_line *find_line(unsigned tag, int i)
{
    cache_line *line = moesi_cache[i].LRU_head;
    while (line)
    {
        if (line->tag == tag)
        {
            break;
        }
        line = line->LRU_next;
    }
    return line;
}

int read_search(int i, unsigned tag)
{
    int source = SOURCE_MEM;
    for (int j = 0; j < num_core; j++)
    {
        if (j != i)
        {
            cache_line *line = find_line(tag, j);
            if (line)
            {
                switch(line->state)
                {
                    case STATE_MODIFIED:
                        line->state = STATE_OWNED;
                    case STATE_OWNED:
                        source = j;
                        break;
                    case STATE_EXCLUSIVE:
                        line->state = STATE_SHARED;
                    case STATE_SHARED:
                        if (source == SOURCE_MEM)
                            source = j;
                        break;
                }
            }
        }
    }
    moesi_cache_stat[i].broadcasts ++;
    moesi_cache_stat[i].demand_fetches++;

    return source;
}

void evict(int i)
{
    if (moesi_cache_stat[i].evictions == 0)
        printf("FIRST EVICTION AT %d\n MISSES: %d\n", moesi_cache_stat[i].accesses + 1, moesi_cache_stat[i].misses);

    cache_line *tail = moesi_cache[i].LRU_tail;

    if (tail->state == STATE_MODIFIED || tail->state == STATE_OWNED)
        moesi_cache_stat[i].copies_back ++;

    delete(&moesi_cache[i].LRU_head, &moesi_cache[i].LRU_tail, tail);
    moesi_cache[i].cache_contents --;
    moesi_cache_stat[i].evictions ++;
}

void add_line (int i, unsigned tag, int state, cache_line *line)
{
    if (line)
    {
        move_to_front(i, line);
        line->state = state;
    }
    else
    {
        cache_line *new_line = malloc(sizeof(cache_line));
        new_line->tag = tag;
        new_line->state = state;
        if (moesi_cache[i].cache_contents >= cache_usize/cache_block_size)
            evict(i);

        insert(&moesi_cache[i].LRU_head, &moesi_cache[i].LRU_tail, new_line);
        moesi_cache[i].cache_contents ++;
    }
    moesi_cache_stat[i].misses ++;
}

void invalidate(int i, unsigned tag)
{
    for (int j = 0; j < num_core; j ++)
    {
        if (j != i)
        {
            cache_line *line = find_line(tag, j);
            if (line){
               // if (tag == 7095312)
                    //printf("prev state = %d\n", line->state);
                line->state = STATE_INVALID;
                //if (tag == 7095312)
                   // printf("now state = %d\n", line->state);
            }
        }
    }
    moesi_cache_stat[i].broadcasts ++;
}

int write_search(int i, unsigned tag)
{
    int source = SOURCE_MEM;
    for (int j = 0; j < num_core; j ++)
    {
        if (j != i)
        {
            cache_line *line = find_line(tag, j);

            if (line)
            {
                //printf("Other line is %d\n", line->state);
                switch(line->state)
                {
                    case STATE_MODIFIED:
                    case STATE_OWNED:
                        source = j;
                    case STATE_EXCLUSIVE:
                    case STATE_SHARED:
                        if (source == SOURCE_MEM)
                            source = j;
                }
            }
        }
    }
    moesi_cache_stat[i].broadcasts ++;
    moesi_cache_stat[i].demand_fetches ++;

    return source;
}

void perform_access_store(int addr, int i)
{
    unsigned tag = calculate_tag(addr);
    cache_line *line = find_line(tag, i);
    //if (tag == 7095312)
    //{
        //printf("\n");
       // print_caches();
        //if (line)
            //printf("Found line in cache %d\n", i);
    //}
    if (line && line->state != STATE_INVALID)
    {
        move_to_front(i, line);
        if (line->state != STATE_EXCLUSIVE && line->state != STATE_MODIFIED)
        {
            invalidate(i, tag);
        }
        else 
        { 
            //printf("\ntag is %d\n", tag);
            //printf("State is: %d in %d\n", line->state, i);
            //printf("Other caches which have it %d\n", write_search(i, tag));
            //sleep(1);
            
        }
        line->state = STATE_MODIFIED;
    }
    else
    {
        int source = write_search(i, tag);
        if (source != SOURCE_MEM)
        {
            moesi_cache_stat[source].reply_fetches ++;
            invalidate(i, tag);
        }
        add_line(i, tag, STATE_MODIFIED, line);

    }
    moesi_cache_stat[i].accesses ++;
}

void perform_access_load(int addr, int i)
{
    /* find line in cache */
    unsigned tag = calculate_tag(addr);

    cache_line *line = find_line(tag, i);

    if (line && line->state != STATE_INVALID)
    {  
       move_to_front(i, line);
    }
    else
    {
        /* read miss */

        // track if we initialize using shared or exclusive (depends on if other caches have the same line)
        int source = read_search(i, tag);

        if (source == SOURCE_MEM)
        {
            // no other cache contains this line
            add_line(i, tag, STATE_EXCLUSIVE, line);
        }
        else
        {
            add_line(i, tag, STATE_SHARED, line);
            // other caches have this line but it is clean
            moesi_cache_stat[source].reply_fetches++;
           
        }
    }

    moesi_cache_stat[i].accesses++;
}

