/* Deep links: a rommdeck:// URL that names one game.
 *
 * The OS handler passes the URL on the command line, and a copy of the app that
 * is already running is told over its own API instead. Both end up here as one
 * pending target, which the web UI takes once and navigates to.
 */
#ifndef ROMM_DEEPLINK_H
#define ROMM_DEEPLINK_H

#include <stddef.h>

/* Reads rom=, name= and platform= out of a URL and leaves them pending.
 * Returns 1 when the URL named a game, 0 when it didn't. */
int deeplink_parse(const char *url);

/* Leaves a target pending, as the API route does for a running copy. */
void deeplink_set(int rom, const char *name, const char *platform);

/* Takes the pending target, clearing it so the UI only navigates once.
 * Returns 1 when there was one. */
int deeplink_take(int *rom, char *name, size_t name_len, char *platform, size_t platform_len);

/* 1 when a target is waiting, for logging. */
int deeplink_pending(void);

#endif
