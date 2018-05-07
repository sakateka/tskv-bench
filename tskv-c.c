#define PCRE2_STATIC
#define PCRE2_CODE_UNIT_WIDTH 8
#define _GNU_SOURCE

#include <stdio.h>
#include <fcntl.h>
#include <stdbool.h>
#include <pcre2.h>
#include "ht.c"

// assume 30 fields in tskv log but hashtable can hold more more keys
// by chaining at collision
#define DEFAULT_FIELDS_NUM 30

typedef struct Index_t {
    size_t keyLen;
    char   *valueOffset;
    char   *start;
    char   *end;
} Index_t;

typedef struct Indexer_t {
    bool    initialized;
    char    *line;
    ssize_t size;
    cht     *fieldIndexes;
    Index_t **i;
} Indexer_t;

int parseFields(Indexer_t *i) {
    char *cursor = i->line;
    char *buf_end = i->line + i->size;
    if (memmem(i->line, 5, "tskv\t", 5) == NULL) {
        return 0;
    }
    // strip '\n'
    *buf_end = '\0';
    buf_end--;

    int length = 0;
    // tskv\t skiped and i.i[0] is first key=value field
    while ((cursor = memchr(cursor, '\t', buf_end - cursor))){
        *cursor = '\0'; // split by fields;
        char *key = ++cursor;
        if (! i->initialized) {
            if (length != 0) {
                i->i = realloc(i->i, sizeof(Index_t *)*length+1);
            }
            i->i[length] = calloc(1, sizeof(Index_t));
            cursor = memchr(cursor, '=', buf_end - cursor);
            *cursor = '\0'; // split name and value
            i->i[length]->keyLen = cursor - key;
            cht_set(i->fieldIndexes, key, length);
        } else {
            cursor += i->i[length]->keyLen;
            *cursor = '\0'; // split name and value
        }
        i->i[length]->valueOffset = ++cursor;
        i->i[length]->start = key;
        if (length != 0) {
            i->i[length-1]->end = key - 1;
        }
        length++;
    }
    //if (! i->initialized) {
    //    cht_print(i->fieldIndexes);
    //}
    i->initialized = true;
    i->i[length-1]->end = buf_end - 1;
    return length + 1; // +1 for count leading tskv\t
}

Index_t* v(Indexer_t *i, char *name) {
    // TODO: error check;
    return i->i[cht_get(i->fieldIndexes, name)->data.value];
}

pcre2_code* compile_re() {
    int errornumber;
    PCRE2_SIZE erroroffset;

    PCRE2_SPTR pattern = (PCRE2_SPTR)"^/[^/?:.-]{3,6}/";
    //PCRE2_SPTR pattern = (PCRE2_SPTR)"^(/[^/?:.-][^/?:.-][^/?:.-][^/?:.-]?[^/?:.-]?[^/?:.-]?/)";
    pcre2_code *re = pcre2_compile(
        pattern,               /* the pattern */
        PCRE2_ZERO_TERMINATED, /* indicates pattern is zero‐terminated */
        0,                     /* default options */
        &errornumber,          /* for error number */
        &erroroffset,          /* for error offset */
        NULL                   /* use default compile context */
    );
    if (re == NULL) {
        PCRE2_UCHAR buffer[256];
        pcre2_get_error_message(errornumber, buffer, sizeof(buffer));
        printf("PCRE2 compilation failed at offset %d: %s\n", (int)erroroffset, buffer);
        return NULL;
    }
    return re;
}

typedef struct match_result {
    char *substring_start;
    int substring_length;
} match_result;

match_result* request_re_find(Indexer_t *i, pcre2_code *re, pcre2_match_data *mdata, match_result *r) {
    Index_t *request_info = v(i, "request");

    PCRE2_SPTR subject = (PCRE2_SPTR)request_info->valueOffset;
    size_t subject_length = request_info->end - request_info->valueOffset;
    int rc = pcre2_match(
        re,                   /* the compiled pattern */
        subject,              /* the subject string */
        subject_length,       /* the length of the subject */
        0,                    /* start at offset 0 in the subject */
        0,                    /* default options */
        mdata,                /* block for storing the result */
        NULL                  /* use default match context */
    );
    if (rc < 0) {
        switch(rc) {
            case PCRE2_ERROR_NOMATCH:
                //printf("No match\n");
                break;
            /*
            Handle other special cases if you like
            */
            default:
                //printf("Matching error %d\n", rc);
                break;
        }
        // reuse match_data and re;
        //pcre2_match_data_free(match_data);   /* Release memory used for the match */
        //pcre2_code_free(re);                 /* data and the compiled pattern. */
        return NULL;
    }
    PCRE2_SIZE *ovector = pcre2_get_ovector_pointer(mdata);
    //printf("\nMatch succeeded at offset %d\n", (int)ovector[0]);

    // capture match with index 0 is substring matched with full re
    r->substring_start = (char*)(subject + ovector[0]);
    r->substring_length = (int)(ovector[1] - ovector[0]);
    return r;
}

int main(int argc, char *argv[]) {

    char counter_key[80] = {0}; // All will suffice

    pcre2_code *re = compile_re();
    if (re == NULL) {
        return 1;
    }
    match_result *mr = calloc(1, sizeof(match_result));
    pcre2_match_data *match_data = pcre2_match_data_create_from_pattern(re, NULL);

    Indexer_t *layout = calloc(1, sizeof(Indexer_t));
    layout->i = calloc(1, sizeof(Index_t *));
    layout->fieldIndexes = cht_init(DEFAULT_FIELDS_NUM);
    cht* matched_request = cht_init(DEFAULT_FIELDS_NUM);

    FILE *logFile = fopen("access.log", "r");
    if (!logFile) {
        perror("Failed to open file");
    }
    ssize_t size = 0;
    size_t len = 1<<20;
    char *buf = malloc(len); // 1 mb

    cht *table = cht_init(1);
    while ((size = getline(&buf, &len, logFile)) != -1){
        layout->line = buf;
        layout->size = size;
        int line_len = parseFields(layout);
        snprintf(counter_key, 80, "%d", line_len);
        list_node *counter = cht_get(table, counter_key);
        if (counter == NULL) {
            cht_set(table, counter_key, 1);
        } else {
            counter->data.value++;
        }

        if (request_re_find(layout, re, match_data, mr) == NULL) {
            continue;
        }
        // ugly hack for speedup :-) avoid strcpy
        *(mr->substring_start + mr->substring_length) = '\0';
        list_node *request_counter = cht_get(matched_request, mr->substring_start);
        if (request_counter == NULL) {
            cht_set(matched_request, mr->substring_start, 1);
        } else {
            request_counter->data.value++;
        }
    }
    cht_print(table);
    cht_print(matched_request);
    return 0;
}
