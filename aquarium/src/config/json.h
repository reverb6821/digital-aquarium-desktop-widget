// Tiny allocation-free JSON tokenizer (jsmn-style) + string builder.
// Enough for the config/preset files; strings are not unescaped beyond \" and \\.
#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum { JT_UNDEF = 0, JT_OBJ, JT_ARR, JT_STR, JT_PRIM } JsonType;

typedef struct {
    JsonType type;
    int start, end; // byte range in the source (strings exclude quotes)
    int size;       // children: object -> key count, array -> element count
} JsonTok;

// Returns number of tokens, or -1 on error / too many tokens.
int json_parse(const char *js, JsonTok *t, int max);
// Index of the token following the subtree rooted at i.
int json_next(const JsonTok *t, int i);
// Value token index for `key` inside object at `obj`, or -1.
int json_get(const char *js, const JsonTok *t, int obj, const char *key);
// Index of element n inside array at `arr`, or -1.
int json_at(const JsonTok *t, int arr, int n);
long json_int(const char *js, const JsonTok *t, int i, long def);
bool json_bool(const char *js, const JsonTok *t, int i, bool def);
void json_str(const char *js, const JsonTok *t, int i, char *out, size_t cap);

typedef struct {
    char *buf;
    size_t cap, len;
    bool overflow;
} JsonWriter;

void jw_init(JsonWriter *w, char *buf, size_t cap);
void jw_raw(JsonWriter *w, const char *s);
void jw_printf(JsonWriter *w, const char *fmt, ...);
void jw_str(JsonWriter *w, const char *s); // quoted + escaped
