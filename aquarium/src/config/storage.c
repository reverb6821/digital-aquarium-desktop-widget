#include "storage.h"
#include <stdio.h>
#include <string.h>
#include "../engine/hal.h"

#define DEBOUNCE_MS 2000
#define ACTIVE_KEY "cfgj" // NVS record holding the active config as JSON

typedef enum { REQ_NONE = 0, REQ_SAVE, REQ_DELETE } ReqType;

static struct {
    bool hold;
    uint32_t write_after; // after a release, all pending writes run together at this time
    uint32_t last_hash; // FNV-1a of the last JSON written (skip identical saves)
    bool active_dirty;
    uint32_t active_due;
    AquariumConfig active; // pending copy
    // pending preset operations, one per slot (they can pile up while held)
    ReqType preset_req[CFG_MAX_USER_PRESETS];
    char preset_name_pending[CFG_MAX_USER_PRESETS][PRESET_NAME_LEN];
    AquariumConfig preset[CFG_MAX_USER_PRESETS];
    char names[CFG_MAX_USER_PRESETS][PRESET_NAME_LEN];
} st;

static char json_buf[6144]; // used only by the writer (storage task)
static char load_buf[6144]; // used only by readers (boot / render task)

static void preset_path(int slot, char *out, size_t cap) { snprintf(out, cap, "/preset%d.json", slot); }

void storage_init(void)
{
    memset(&st, 0, sizeof(st));
    for (int i = 0; i < CFG_MAX_USER_PRESETS; i++) {
        char path[32];
        preset_path(i, path, sizeof(path));
        if (hal_file_read(path, load_buf, sizeof(load_buf)) == 0) continue;
        AquariumConfig tmp;
        memset(&tmp, 0, sizeof(tmp));
        char name[PRESET_NAME_LEN];
        if (config_from_json(&tmp, load_buf, name, sizeof(name))) {
            if (!name[0]) snprintf(name, sizeof(name), "Preset %d", i + 1);
            memcpy(st.names[i], name, sizeof(name));
        }
    }
}

static uint32_t fnv1a(const char *s, size_t n)
{
    uint32_t h = 2166136261u;
    for (size_t i = 0; i < n; i++) h = (h ^ (uint8_t)s[i]) * 16777619u;
    return h;
}

bool storage_load_active(AquariumConfig *c)
{
    size_t n = hal_blob_load(ACTIVE_KEY, load_buf, sizeof(load_buf) - 1);
    if (n > 0) {
        load_buf[n] = 0;
        AquariumConfig tmp = *c;
        if (config_from_json(&tmp, load_buf, NULL, 0)) {
            *c = tmp;
            st.last_hash = fnv1a(load_buf, n);
            return true;
        }
    }
    // Older firmware kept a copy in LittleFS.
    if (hal_file_read("/active.json", load_buf, sizeof(load_buf)) > 0) {
        AquariumConfig tmp = *c;
        if (config_from_json(&tmp, load_buf, NULL, 0)) {
            *c = tmp;
            return true;
        }
    }
    return false;
}

void storage_hold(bool hold, uint32_t now)
{
    hal_lock();
    if (st.hold && !hold) st.active_due = st.write_after = now + DEBOUNCE_MS; // released: save a bit later
    st.hold = hold;
    hal_unlock();
}

void storage_mark_dirty(const AquariumConfig *c, uint32_t now)
{
    hal_lock();
    st.active = *c;
    st.active_dirty = true;
    st.active_due = now + DEBOUNCE_MS;
    hal_unlock();
}

void storage_request_preset_save(int slot, const AquariumConfig *c, const char *name)
{
    if (slot < 0 || slot >= CFG_MAX_USER_PRESETS) return;
    hal_lock();
    st.preset[slot] = *c;
    st.preset_req[slot] = REQ_SAVE;
    snprintf(st.preset_name_pending[slot], PRESET_NAME_LEN, "%s", name);
    snprintf(st.names[slot], PRESET_NAME_LEN, "%s", name); // UI sees it immediately
    hal_unlock();
}

void storage_request_preset_delete(int slot)
{
    if (slot < 0 || slot >= CFG_MAX_USER_PRESETS) return;
    hal_lock();
    st.preset_req[slot] = REQ_DELETE;
    st.names[slot][0] = 0;
    hal_unlock();
}

bool storage_preset_load(int slot, AquariumConfig *c)
{
    if (slot < 0 || slot >= CFG_MAX_USER_PRESETS || !st.names[slot][0]) return false;
    char path[32];
    preset_path(slot, path, sizeof(path));
    hal_lock();
    bool pending = st.preset_req[slot] == REQ_SAVE; // not written yet
    if (pending) *c = st.preset[slot];
    hal_unlock();
    if (pending) return true;
    if (hal_file_read(path, load_buf, sizeof(load_buf)) == 0) return false;
    return config_from_json(c, load_buf, NULL, 0);
}

const char *storage_preset_name(int slot)
{
    if (slot < 0 || slot >= CFG_MAX_USER_PRESETS) return "";
    return st.names[slot];
}

static void do_writes(uint32_t now, bool force)
{
    static AquariumConfig work;
    static char name[PRESET_NAME_LEN];

    hal_lock();
    // menu / edit mode open: no flash writes (they glitch the LCD)
    if (!force && (st.hold || (int32_t)(now - st.write_after) < 0)) {
        hal_unlock();
        return;
    }
    bool save_active = st.active_dirty && (force || (int32_t)(now - st.active_due) >= 0);
    if (save_active) {
        work = st.active;
        st.active_dirty = false;
    }
    hal_unlock();
    if (save_active) {
        size_t n = config_to_json(&work, "active", json_buf, sizeof(json_buf));
        uint32_t h = n ? fnv1a(json_buf, n) : 0;
        if (n && h != st.last_hash && hal_blob_save(ACTIVE_KEY, json_buf, n)) st.last_hash = h;
    }

    for (int slot = 0; slot < CFG_MAX_USER_PRESETS; slot++) {
        hal_lock();
        ReqType req = st.preset_req[slot];
        if (req == REQ_SAVE) {
            work = st.preset[slot];
            memcpy(name, st.preset_name_pending[slot], sizeof(name));
        }
        st.preset_req[slot] = REQ_NONE;
        hal_unlock();
        if (req == REQ_NONE) continue;
        char path[32];
        preset_path(slot, path, sizeof(path));
        if (req == REQ_SAVE) {
            size_t n = config_to_json(&work, name, json_buf, sizeof(json_buf));
            if (n) hal_file_write(path, json_buf, n);
        } else {
            hal_file_remove(path);
        }
    }
}

void storage_poll(uint32_t now) { do_writes(now, false); }
void storage_flush(void) { do_writes(0, true); }
