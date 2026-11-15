#include "json.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ---------------------------------------------------------------- tokenizer

typedef struct {
    const char *js;
    int pos;
    JsonTok *t;
    int n, max;
} Parser;

static int alloc_tok(Parser *p, JsonType type, int start, int end)
{
    if (p->n >= p->max) return -1;
    p->t[p->n] = (JsonTok){type, start, end, 0};
    return p->n++;
}

static void skip_ws(Parser *p)
{
    while (p->js[p->pos] == ' ' || p->js[p->pos] == '\n' || p->js[p->pos] == '\r' || p->js[p->pos] == '\t') p->pos++;
}

static int parse_value(Parser *p);

static int parse_string(Parser *p)
{
    int start = ++p->pos; // skip opening quote
    while (p->js[p->pos] && p->js[p->pos] != '"') {
        if (p->js[p->pos] == '\\' && p->js[p->pos + 1]) p->pos++;
        p->pos++;
    }
    if (p->js[p->pos] != '"') return -1;
    int i = alloc_tok(p, JT_STR, start, p->pos);
    p->pos++;
    return i;
}

static int parse_container(Parser *p, bool obj)
{
    int i = alloc_tok(p, obj ? JT_OBJ : JT_ARR, p->pos, -1);
    if (i < 0) return -1;
    char close = obj ? '}' : ']';
    p->pos++;
    skip_ws(p);
    if (p->js[p->pos] == close) {
        p->t[i].end = ++p->pos;
        return i;
    }
    for (;;) {
        skip_ws(p);
        if (obj) {
            if (p->js[p->pos] != '"' || parse_string(p) < 0) return -1;
            skip_ws(p);
            if (p->js[p->pos] != ':') return -1;
            p->pos++;
        }
        if (parse_value(p) < 0) return -1;
        p->t[i].size++;
        skip_ws(p);
        if (p->js[p->pos] == ',') { p->pos++; continue; }
        if (p->js[p->pos] == close) { p->t[i].end = ++p->pos; return i; }
        return -1;
    }
}

static int parse_value(Parser *p)
{
    skip_ws(p);
    char c = p->js[p->pos];
    if (c == '{') return parse_container(p, true);
    if (c == '[') return parse_container(p, false);
    if (c == '"') return parse_string(p);
    int start = p->pos;
    while (p->js[p->pos] && !strchr(",}] \t\r\n", p->js[p->pos])) p->pos++;
    if (p->pos == start) return -1;
    return alloc_tok(p, JT_PRIM, start, p->pos);
}

int json_parse(const char *js, JsonTok *t, int max)
{
    Parser p = {js, 0, t, 0, max};
    if (parse_value(&p) < 0) return -1;
    return p.n;
}

int json_next(const JsonTok *t, int i)
{
    switch (t[i].type) {
    case JT_OBJ: {
        int j = i + 1;
        for (int k = 0; k < t[i].size; k++) {
            j = json_next(t, j); // key
            j = json_next(t, j); // value
        }
        return j;
    }
    case JT_ARR: {
        int j = i + 1;
        for (int k = 0; k < t[i].size; k++) j = json_next(t, j);
        return j;
    }
    default: return i + 1;
    }
}

int json_get(const char *js, const JsonTok *t, int obj, const char *key)
{
    if (obj < 0 || t[obj].type != JT_OBJ) return -1;
    size_t kl = strlen(key);
    int j = obj + 1;
    for (int k = 0; k < t[obj].size; k++) {
        int len = t[j].end - t[j].start;
        if ((size_t)len == kl && memcmp(js + t[j].start, key, kl) == 0) return j + 1;
        j = json_next(t, j);
        j = json_next(t, j);
    }
    return -1;
}

int json_at(const JsonTok *t, int arr, int n)
{
    if (arr < 0 || t[arr].type != JT_ARR || n >= t[arr].size) return -1;
    int j = arr + 1;
    for (int k = 0; k < n; k++) j = json_next(t, j);
    return j;
}

long json_int(const char *js, const JsonTok *t, int i, long def)
{
    if (i < 0) return def;
    if (t[i].type == JT_STR && js[t[i].start] == '#') // "#rrggbb" colour -> RGB565
    {
        unsigned long v = strtoul(js + t[i].start + 1, NULL, 16);
        unsigned r = (v >> 16) & 255, g = (v >> 8) & 255, b = v & 255;
        return (long)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
    }
    if (t[i].type != JT_PRIM) return def;
    char *end;
    long v = strtol(js + t[i].start, &end, 10);
    return end == js + t[i].start ? def : v;
}

bool json_bool(const char *js, const JsonTok *t, int i, bool def)
{
    if (i < 0 || t[i].type != JT_PRIM) return def;
    if (js[t[i].start] == 't') return true;
    if (js[t[i].start] == 'f') return false;
    return json_int(js, t, i, def ? 1 : 0) != 0;
}

void json_str(const char *js, const JsonTok *t, int i, char *out, size_t cap)
{
    if (!cap) return;
    out[0] = 0;
    if (i < 0 || t[i].type != JT_STR) return;
    size_t o = 0;
    for (int k = t[i].start; k < t[i].end && o + 1 < cap; k++) {
        char c = js[k];
        if (c == '\\' && k + 1 < t[i].end) c = js[++k];
        out[o++] = c;
    }
    out[o] = 0;
}

// ---------------------------------------------------------------- writer

void jw_init(JsonWriter *w, char *buf, size_t cap)
{
    w->buf = buf;
    w->cap = cap;
    w->len = 0;
    w->overflow = false;
    if (cap) buf[0] = 0;
}

void jw_printf(JsonWriter *w, const char *fmt, ...)
{
    if (w->overflow) return;
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(w->buf + w->len, w->cap - w->len, fmt, ap);
    va_end(ap);
    if (n < 0 || (size_t)n >= w->cap - w->len) {
        w->overflow = true;
        return;
    }
    w->len += (size_t)n;
}

void jw_raw(JsonWriter *w, const char *s) { jw_printf(w, "%s", s); }

void jw_str(JsonWriter *w, const char *s)
{
    jw_raw(w, "\"");
    for (; *s; s++) {
        if (*s == '"' || *s == '\\') jw_printf(w, "\\%c", *s);
        else if ((unsigned char)*s >= 0x20) jw_printf(w, "%c", *s);
    }
    jw_raw(w, "\"");
}
