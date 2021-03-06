#define TRUE 1
#define FALSE 0

/* default cache parameters--can be changed */
#define WORD_SIZE 4
#define WORD_SIZE_OFFSET 2
#define DEFAULT_CACHE_SIZE (8 * 1024)
#define DEFAULT_CACHE_BLOCK_SIZE 16
#define DEFAULT_CACHE_WRITEBACK TRUE
#define DEFAULT_CACHE_WRITEALLOC TRUE
#define DEFAULT_NUM_CORE 2

/* constants for setting cache parameters */
#define NUM_CORE 0
#define CACHE_PARAM_BLOCK_SIZE 1
#define CACHE_PARAM_USIZE 2

/* Defs for cache states */
#define STATE_MODIFIED 4
#define STATE_OWNED 3
#define STATE_EXCLUSIVE 2
#define STATE_SHARED 1
#define STATE_INVALID 0

#define SOURCE_MEM -1

/* structure definitions */
typedef struct cache_line_
{
    unsigned tag;
    int state;
    struct cache_line_ *LRU_next;
    struct cache_line_ *LRU_prev;
} cache_line, *Pcache_line;

typedef struct cache_
{
    int id;               /* core ID */
    int size;             /* cache size */
    Pcache_line LRU_head; /* head of LRU list for the cache */
    Pcache_line LRU_tail; /* tail of LRU list for the cache */
    int cache_contents;   /* number of valid entries in cache */
} cache, *Pcache;

typedef struct cache_stat_
{
    int accesses;       /* number of memory references */
    int misses;         /* number of cache misses */
    int evictions;      /* number of misses that cause evictions (such as removing LRU from cache) */
    int demand_fetches; /* number of requests for data originating from this cache */
    int reply_fetches;  /* number of responses for data requests originating from other caches */
    int copies_back;    /* number of write backs to memory */
    int broadcasts;     /* number of broadcasts */
} cache_stat, *Pcache_stat;

/* function prototypes */
void set_cache_param();
void init_cache();
void perform_access();
void flush();
void delete ();
void insert();
void dump_settings();
void print_stats();
void init_stat(Pcache_stat stat);
void perform_access_store(int addr, int i);
void perform_access_load(int addr, int i);
unsigned calculate_tag(unsigned address);
unsigned round_up(unsigned num, unsigned base);
unsigned round_down(unsigned num, unsigned base);

/* macros */
#define LOG2(x) ((int)(log((double)(x)) / log(2)))
