#ifndef __MAP_H__
#define __MAP_H__

typedef struct map map;
typedef struct map_iterator map_iterator;

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
int map_put(map *m, long long key, void *value);

/*
 * Gets the item associated with the given key in the given map.
 * Returns NULL if no such key is in the map.
 */
void* map_get(map *m, long long key);

/*
 * Removes and returns the item associated with the given key in the given map.
 * Returns NULL if no such key is in the map.
 */
void* map_remove(map *m, long long key);

/*
 * Returns the number of pairs in the map.
 */
int map_size(map *m);

/*
 * Returns a pointer to a new iterator for the given map or NULL on error.
 */
map_iterator* map_iterator_new(map *m);

/*
 * Returns a pointer to the next value in the map, or NULL if there are no
 * remaining items.  Places the key corresponding to the given value in *key
 * if key is not NULL.
 */
void* map_iterator_next(map_iterator *iterator, long long *key);

/*
 * Removes the last item returned by the iterator.
 * Does nothing if map_iterator_next() has not been called at least once.
 */
void map_iterator_remove(map_iterator *iterator);

/*
 * Empties and frees the iterator.
 */
void map_iterator_free(map_iterator *iterator);

#endif
