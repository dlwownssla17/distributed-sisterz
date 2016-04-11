#ifndef __MAP_H__
#define __MAP_H__

typedef struct map map;

/*
 * Returns a pointer to an empty map or NULL on error.
 */
map* map_new();

/*
 * Empties and frees the map.
 */
void map_free(map *m);

/*
 * Associates the given value with the given key in the given map.
 * If a value was already associated with that key, it will be replaced.
 * Returns 0 on success or -1 on failure.
 */
int map_put(map *m, int key, void *value);

/*
 * Gets the item associated with the given key in the given map.
 * Returns NULL if no such key is in the map.
 */
void* map_get(map *m, int key);

/*
 * Removes and returns the item associated with the given key in the given map.
 * Returns NULL if no such key is in the map.
 */
void* map_remove(map *m, int key);

#endif
