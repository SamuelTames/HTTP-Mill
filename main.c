// November 24, 2015 - Samuel Tames
// this file relies on POSIX libraries
#include <stdio.h> // FILE, getc_unlocked, ftello, fseeko, fopen
#include <pthread.h> // pthread_*
#include <assert.h> // assert
#include <errno.h> // errno/perror
#include <stdlib.h> // malloc
#include <sys/stat.h> // stat

#include "header_counter_lib/header_counter_header_code.h"

// NUM_THREADS = $NUM_CPUS is probably a good starting point, increase for more IO.
#define NUM_THREADS 4
// http header terminal = '\r' + '\n'
#define TERMINAL_LENGTH 2
#define GOTO_PERROR_ON_STATUS(status, str)      \
    if ((status) != 0) {                        \
        err_str = (str);                        \
        goto perror;                            \
    } else { (void) 0; }

static bool is_header_terminal(char first, char second);
static bool advance_to_next_header(FILE * file);
static bool process_header_name(HeaderCounter * header_counter, FILE * file);
void * process_chunk(void * context);
static off_t get_filesize(char const * filename);

typedef struct {
    pthread_t thread;
    char const * filename;
    off_t start;
    off_t end;
    struct {
        HeaderCounter * gbl_hdr_ctr;
        pthread_mutex_t * gbl_hdr_ctr_lock;
    };
} ChunkContext;


static inline bool
is_header_terminal(char first, char second)
{
    return first == '\r' && second == '\n';
}

static bool
advance_to_next_header(FILE * file)
{
    assert(file != NULL);

    bool file_done = false;

    int cur = getc_unlocked(file);
    int next = getc_unlocked(file);

    while (next != EOF) {
        if (is_header_terminal((char) cur, (char) next)) {
            break;
        }
        cur = next;
        next = getc_unlocked(file);
    }

    if (next == EOF) {
        file_done = true;
    }

    return file_done;
}

static bool
process_header_name(HeaderCounter * header_counter, FILE * file)
{
    // + 1 reserves space for null termination.
    char header[MAX_HEADER_LENGTH + 1];
    bool file_done = false;
    int c = 0;
    size_t i = 0;

    for (i = 0; i < sizeof(header); i += 1){
        c = getc_unlocked(file);
        if (c == EOF) {
            file_done = true;
            break;
        }
        header[i] = (char) c;
        if (header[i] == ':') {
            header[i] = '\0';
            HdrCtr_increment(header_counter, (char const *) header);
            break;
        }
    }

    return file_done;
}

void *
process_chunk(void * context)
{
    int status = EXIT_SUCCESS;
    char const * err_str = NULL;

    ChunkContext * chunk = (ChunkContext *)context;

    assert(chunk != NULL);
    assert(chunk->filename != NULL);
    assert(chunk->gbl_hdr_ctr != NULL);
    assert(chunk->gbl_hdr_ctr_lock != NULL);
    assert(chunk->start < chunk->end);
    assert(chunk->start >= 0);

    FILE * const file = fopen(chunk->filename, "r");
    if(file == NULL) {
        status = -1;
        GOTO_PERROR_ON_STATUS(status, "fopen");
    }
    status = fseeko(file, chunk->start, SEEK_SET);
    GOTO_PERROR_ON_STATUS(status, "fseeko");


    // skip through first header fragment(the prev thread will take ownership
    // of this and combine it with its own fragment)
    if (chunk->start > 0) {
        advance_to_next_header(file);
    }

    HeaderCounter * const header_counter = HdrCtr_new();
    bool file_done = false;
    off_t pos = ftello(file);
    while (pos < chunk->end + TERMINAL_LENGTH) {
        if (pos == -1) {
            status = pos;
            GOTO_PERROR_ON_STATUS(status, "ftell");
        }

        file_done = process_header_name(header_counter, file);
        if (file_done){ break; }

        file_done = advance_to_next_header(file);
        if (file_done) { break; }

        pos = ftello(file);
    }


    status = pthread_mutex_lock(chunk->gbl_hdr_ctr_lock);
    GOTO_PERROR_ON_STATUS(status, "pthread_mutex_lock");

    HdrCtr_merge_counters(chunk->gbl_hdr_ctr, header_counter);

    status = pthread_mutex_unlock(chunk->gbl_hdr_ctr_lock);
    GOTO_PERROR_ON_STATUS(status, "pthread_mutex_unlock");


    status = fclose(file);
    GOTO_PERROR_ON_STATUS(status, "fclose");

 perror:
    if (status != 0) {
        perror(err_str);
        exit(errno);
    }

    HdrCtr_free(header_counter);

    return context;
}

// this may fail when the file is a block device(ie /dev/*)
// this shouldn't be of issue with the task given though.
static inline off_t
get_filesize(char const * filename)
{
    assert(filename != NULL);

    struct stat st;
    off_t filesize = -1;
    int status = 0;

    status = stat(filename, &st);
    if (status != 0) {
        perror("stat");
        exit(errno);
    }

    filesize = st.st_size;

    return filesize;
}

int
main(int argc, char ** argv)
{
    assert(NUM_THREADS > 0);

    int status = EXIT_SUCCESS;
    char const * err_str = NULL;
    FILE * output_stream = stdout;

    if (argc != 2) {
        fprintf(output_stream, "usage: %s <filename>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    char const * filename = argv[1];

    off_t file_size = get_filesize(filename);
    off_t chunk_size = file_size / NUM_THREADS;

    HeaderCounter * const gbl_hdr_ctr = HdrCtr_new();
    pthread_mutex_t gbl_hdr_ctr_lock;
    status = pthread_mutex_init(&gbl_hdr_ctr_lock, NULL);
    GOTO_PERROR_ON_STATUS(status, "pthread_mutex_init");

    int i = 0;
    ChunkContext chunks[NUM_THREADS];
    for (i = 0; i < NUM_THREADS; i += 1) {
        chunks[i].filename = filename;
        chunks[i].start = i * chunk_size;
        chunks[i].end = (i + 1) * chunk_size;
        chunks[i].gbl_hdr_ctr = gbl_hdr_ctr;
        chunks[i].gbl_hdr_ctr_lock = &gbl_hdr_ctr_lock;
    }
    for (i = 0; i < NUM_THREADS; i += 1) {
        status = pthread_create(&(chunks[i].thread), NULL, process_chunk, &chunks[i]);
        GOTO_PERROR_ON_STATUS(status, "pthread_create");
    }
    for (i = 0; i < NUM_THREADS; i += 1) {
        status = pthread_join(chunks[i].thread, NULL);
        GOTO_PERROR_ON_STATUS(status, "pthread_join");
    }

    HdrCtr_print(output_stream, gbl_hdr_ctr);

 perror:
    if (status != 0) {
        perror(err_str);
        exit(errno);
    }

    // But... "why clean the kitchen before burning the house down?"
    HdrCtr_free(gbl_hdr_ctr);
    pthread_mutex_destroy(&gbl_hdr_ctr_lock);

    return status;
}
