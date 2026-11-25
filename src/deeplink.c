/* Deep links: a rommdeck:// URL that names one game. See deeplink.h. */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "deeplink.h"
#include "util.h"

#define DEEPLINK_NAME_LEN 256
#define DEEPLINK_SLUG_LEN 64

static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;
static int g_rom = 0;
static char g_name[DEEPLINK_NAME_LEN] = "";
static char g_platform[DEEPLINK_SLUG_LEN] = "";

/* %XX and + as a URL writes them; anything malformed is copied through. */
static void url_decode(const char *in, char *out, size_t out_len) {
    size_t o = 0;
    for (size_t i = 0; in[i] && o + 1 < out_len; i++) {
        if (in[i] == '%' && in[i + 1] && in[i + 2]) {
            int hi = -1, lo = -1;
            char a = in[i + 1], b = in[i + 2];
            if (a >= '0' && a <= '9') hi = a - '0';
            else if (a >= 'a' && a <= 'f') hi = a - 'a' + 10;
            else if (a >= 'A' && a <= 'F') hi = a - 'A' + 10;
            if (b >= '0' && b <= '9') lo = b - '0';
            else if (b >= 'a' && b <= 'f') lo = b - 'a' + 10;
            else if (b >= 'A' && b <= 'F') lo = b - 'A' + 10;
            if (hi >= 0 && lo >= 0) {
                out[o++] = (char)((hi << 4) | lo);
                i += 2;
                continue;
            }
        }
        out[o++] = in[i] == '+' ? ' ' : in[i];
    }
    out[o] = 0;
}

/* The value of ?key= or &key= in a URL, decoded; 0 when the key isn't there. */
static int query_value(const char *url, const char *key, char *out, size_t out_len) {
    size_t klen = strlen(key);
    for (const char *p = strchr(url, '?'); p; p = strchr(p + 1, '?')) {
        const char *s = p + 1;
        while (s && *s) {
            const char *amp = strchr(s, '&');
            size_t flen = amp ? (size_t)(amp - s) : strlen(s);
            if (flen > klen && !strncmp(s, key, klen) && s[klen] == '=') {
                char value[DEEPLINK_NAME_LEN * 4];
                size_t vlen = flen - klen - 1;
                if (vlen >= sizeof value) vlen = sizeof value - 1;
                memcpy(value, s + klen + 1, vlen);
                value[vlen] = 0;
                url_decode(value, out, out_len);
                return 1;
            }
            if (!amp) break;
            s = amp + 1;
        }
    }
    return 0;
}

int deeplink_parse(const char *url) {
    if (!url || !strstr(url, "rom=")) return 0;
    char value[DEEPLINK_NAME_LEN * 4];
    int rom = 0;
    if (!query_value(url, "rom", value, sizeof value)) return 0;
    rom = atoi(value);
    if (rom <= 0) return 0;

    char name[DEEPLINK_NAME_LEN] = "", platform[DEEPLINK_SLUG_LEN] = "";
    if (!query_value(url, "name", name, sizeof name))
        query_value(url, "game", name, sizeof name);
    query_value(url, "platform", platform, sizeof platform);

    deeplink_set(rom, name, platform);
    LOGI("deep link: rom %d%s%s", rom, name[0] ? " " : "", name);
    return 1;
}

void deeplink_set(int rom, const char *name, const char *platform) {
    pthread_mutex_lock(&g_lock);
    g_rom = rom > 0 ? rom : 0;
    str_copy(g_name, sizeof g_name, name ? name : "");
    str_copy(g_platform, sizeof g_platform, platform ? platform : "");
    pthread_mutex_unlock(&g_lock);
}

int deeplink_take(int *rom, char *name, size_t name_len, char *platform, size_t platform_len) {
    int had;
    pthread_mutex_lock(&g_lock);
    had = g_rom > 0;
    if (had) {
        if (rom) *rom = g_rom;
        if (name) str_copy(name, name_len, g_name);
        if (platform) str_copy(platform, platform_len, g_platform);
        g_rom = 0;
        g_name[0] = 0;
        g_platform[0] = 0;
    }
    pthread_mutex_unlock(&g_lock);
    return had;
}

int deeplink_pending(void) {
    int pending;
    pthread_mutex_lock(&g_lock);
    pending = g_rom > 0;
    pthread_mutex_unlock(&g_lock);
    return pending;
}
