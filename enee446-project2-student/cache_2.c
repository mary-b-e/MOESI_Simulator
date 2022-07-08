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
  /*  #ifdef DEBUG
    printf("%d\n", moesi_cache_stat[pid].accesses);
    print_caches();
    getchar();
    #endif
*/
   /* if (moesi_cache_stat[pid].accesses == 18919 || moesi_cache_stat[pid].accesses == 18920 || moesi_cache_stat[pid].accesses == 18921)
    {
        printf("%d\n", moesi_cache_stat[pid].accesses);
        printf("misses: %d\n",moesi_cache_stat[pid].misses);
        printf("Evictions: %d\n", moesi_cache_stat[pid].evictions);
        print_caches();
        getchar();
    }*/
}
/************************************************************/

/************************************************************/

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
    /*printf("MODIFIED \n[");
    for (i = 0; i < num_core; i ++)
    {
        printf("%d", moesi_cache_stat[i].s_m);
        if (i + 1 !=num_core)
            printf(", ");
    }
    printf("]\nOWNED \n[");
    for (i = 0; i < num_core; i ++)
    {
        printf("%d", moesi_cache_stat[i].s_o);
        if (i + 1 !=num_core)
            printf(", ");
    }
    printf("]\nEXCLUSIVE \n[");
    for (i = 0; i < num_core; i ++)
    {
        printf("%d", moesi_cache_stat[i].s_e);
        if (i + 1 !=num_core)
            printf(", ");
    }
    printf("]\nSHARED \n[");
    for (i = 0; i < num_core; i ++)
    {
        printf("%d", moesi_cache_stat[i].s_s);
        if (i + 1 !=num_core)
            printf(", ");
    }
    printf("]\nINVALID\n[");
    for (i = 0; i < num_core; i ++)
    {
        printf("%d", moesi_cache_stat[i].s_i);
        if (i + 1 !=num_core)
            printf(", ");
    }*/
    //printf("]\n");
    printf("%d\n", (demand_fetches)*cache_block_size / WORD_SIZE);
    printf("%d\n", broadcasts);
    printf("%d\n", (reply_fetches)*cache_block_size / WORD_SIZE);
    //printf("%d\n", (copies_back)*cache_block_size / WORD_SIZE);
    printf("%d\n", copies_back);
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

cache_line *find_line_in_cache (int i, unsigned tag)
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

void move_to_front(int i, cache_line *line)
{
    delete(&moesi_cache[i].LRU_head, &moesi_cache[i].LRU_tail, line);
    insert(&moesi_cache[i].LRU_head, &moesi_cache[i].LRU_tail, line);
}

void invalidate(int i, unsigned tag)
{
    //moesi_cache_stat[i].invalidates ++;
    int FOUND = FALSE;
    for (int j = 0; j < num_core; j ++)
    {
        if (j != i)
        {
            cache_line *line = find_line_in_cache(j, tag);
            
            if (line)
            {
                FOUND = TRUE;
                line->state = STATE_INVALID;
                // delete(&moesi_cache[i].LRU_head, &moesi_cache[i].LRU_tail, line);
                // TODO: FIGURE OUT IF I SHOULD DECREASE CACHE_CONTENTS AND DELETE THIS LINE 
            }
        }
    }

    moesi_cache_stat[i].broadcasts ++;
}

void evict (int i)
{
    cache_line *tail = moesi_cache[i].LRU_tail;

    if (tail->state == STATE_MODIFIED || tail->state == STATE_OWNED)
    {
        moesi_cache_stat[i].copies_back ++;
    }
    delete(&moesi_cache[i].LRU_head, &moesi_cache[i].LRU_tail, tail);
    moesi_cache[i].cache_contents --;
    moesi_cache_stat[i].evictions ++;
}

cache_line *create_new_entry(int i, unsigned tag)
{
    cache_line *new_line = malloc(sizeof(cache_line));
    new_line->tag = tag;
    if (moesi_cache[i].cache_contents >= cache_usize/cache_block_size)
    {
        if (moesi_cache_stat[i].evictions == 0){
            printf("Prev misses for %d: %d\n", i, moesi_cache_stat[i].misses);
            printf("Accesses: %d\n", moesi_cache_stat[i].accesses + 1);}
        evict(i);
    }
    insert(&moesi_cache[i].LRU_head, &moesi_cache[i].LRU_tail, new_line);
    moesi_cache[i].cache_contents ++;
    moesi_cache_stat[i].misses ++;
    return new_line;
}

int write_search(int i, unsigned tag)
{
    int source = SOURCE_MEM;
    for (int j = 0; j < num_core; j ++)
    {
        if (j != i)
        {
            cache_line* line = find_line_in_cache(j, tag);

            if (line)
            {
                switch(line->state)
                {
                    case STATE_MODIFIED:
                    case STATE_OWNED:
                        source = j;
                        break;
                    case STATE_EXCLUSIVE:
                    case STATE_SHARED:
                        if (source == SOURCE_MEM)
                        {
                            source = j;
                        }
                        break;
                    default:
                        break;
                }
                line->state = STATE_INVALID;
            }
        }
    }

    moesi_cache_stat[i].broadcasts++;
  //  printf("WRITE SEARCH - FROM WRITE MISS\n");
    if (source != SOURCE_MEM)
    {
        moesi_cache_stat[source].reply_fetches ++;
    }
    moesi_cache_stat[i].demand_fetches++;
    return source;
}

int read_search(int i, unsigned tag)
{
    int source = SOURCE_MEM;
    for (int j = 0; j < num_core; j ++)
    {
        if (j != i)
        {
            cache_line *line = find_line_in_cache(j, tag);

            if (line)
            {
                switch(line->state)
                {
                    case STATE_EXCLUSIVE:
                        line->state = STATE_SHARED;
                    case STATE_SHARED:
                        if (source == SOURCE_MEM)
                        {
                            source = j;
                        }
                        break;
                    case STATE_MODIFIED:
                        line->state = STATE_OWNED;
                    case STATE_OWNED:
                        source = j;
                        break;
                    default:
                        break;
                }
            }
        }
    }
    if (source != SOURCE_MEM)
    {
        moesi_cache_stat[source].reply_fetches ++;
    }
    //printf("READ SEARCH - FROM READ MISS\n");
    moesi_cache_stat[i].broadcasts ++;
    moesi_cache_stat[i].demand_fetches ++;
    return source;
}

void perform_access_store(int addr, int i)
{
    /* find line in cache */
    unsigned tag = calculate_tag(addr);
    //unsigned tag = addr;
    cache_line *line = find_line_in_cache(i, tag);
    if (line && line->state != STATE_INVALID)
    {
        /* write hit */
        move_to_front(i, line);
        if (line->state != STATE_EXCLUSIVE && line->state != STATE_MODIFIED)
        {
            //printf("line state %d\n", line->state);
            invalidate(i, tag);
        }

        
        //moesi_cache_stat[i].evictions ++;
        
        //printf("WRITE HIT\n");
        //if (num_core != 1)
        //    moesi_cache_stat[i].broadcasts ++;
        line->state = STATE_MODIFIED;
     //   moesi_cache_stat[i].write_hit ++;
    }
    else
    {
        if (line && line->state == STATE_INVALID){
            delete(&moesi_cache[i].LRU_head, &moesi_cache[i].LRU_tail, line);
            moesi_cache[i].cache_contents --;
        }
        int source = write_search(i, tag);

//        if (source != SOURCE_MEM)  
            invalidate(i, tag);


        if(line)
            move_to_front(i, line);
        else
            line = create_new_entry(i, tag);
        
        line->state = STATE_MODIFIED;
        
        //invalidate(i, tag);
        //printf("Miss at %d\n", moesi_cache_stat[i].accesses + 1);
        //moesi_cache_stat[i].misses ++;
    }
    moesi_cache_stat[i].accesses ++;
}

void perform_access_load(int addr, int i)
{
    /* find line in cache */
    unsigned tag = calculate_tag(addr);
    //unsigned tag = addr;
    cache_line *line = find_line_in_cache(i, tag);

    if (line && line->state != STATE_INVALID)
    {  
        /* read hit */
        move_to_front(i, line);
    }
    else
    {
        /* read miss */

        // track if we initialize using shared or exclusive (depends on if other caches have the same line)
        int source = read_search(i, tag);
        
        // add the cache line to memory
       /* if (line && line->state == STATE_INVALID){
            delete(&moesi_cache[i].LRU_head, &moesi_cache[i].LRU_tail, line);
            moesi_cache[i].cache_contents --;
        }*/
        if (line)
            move_to_front(i,line);
        else 
            line = create_new_entry(i, tag);
        

        if (source == SOURCE_MEM)
        {
            line->state = STATE_EXCLUSIVE;
        }
        else
        {
            line->state = STATE_SHARED;
        }
        //printf("Miss at %d\n", moesi_cache_stat[i].accesses + 1);
        //moesi_cache_stat[i].misses++;
    }

    moesi_cache_stat[i].accesses++;
}

void flush()
{
    for (int i = 0; i < num_core; i ++)
    {
       // printf("------------------------ CACHE %d -------------------------\n", i);
       // print_cache(i);
        //cache_line *line = ;
        while (moesi_cache[i].LRU_head)
        {
            evict(i);
        //line = line->LRU_next;
        //delete(&moesi_cache[i].LRU_head, &moesi_cache[i].LRU_tail, moesi_cache[i].LRU_tail, FALSE);
        }
    }
}