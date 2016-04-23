#include <stdlib.h>
#include <sys/queue.h>
#include "map.h"

struct map_pair {
	long long key;
	void *value;
	LIST_ENTRY(map_pair) entries;
};

LIST_HEAD(pair_list_head, map_pair);

struct map {
	struct pair_list_head pairs;
	int size;
};

struct map_iterator {
	struct map_pair *current;
	struct map_pair *previous;
	struct map *source;
};



struct map_pair* find_pair_by_key(map *m, long long key)
{
	struct map_pair *current;
	LIST_FOREACH(current, &(m->pairs), entries)
		if(current->key == key) {
			return current;
		}

	return NULL;
}



map* map_new()
{
	map *m = (map *) malloc(sizeof(map));
	if(m != NULL) {
		LIST_INIT(&(m->pairs));
		m->size = 0;
	}
	return m;
}



void map_free(map *m)
{
	struct map_pair *current = LIST_FIRST(&(m->pairs));
	struct map_pair *next;
    while(current != NULL) {
    	next = LIST_NEXT(current, entries);
        free(current);
        current = next;
    }
	free(m);
}



int map_put(map *m, long long key, void *value)
{
	struct map_pair *target_pair = find_pair_by_key(m, key);
	if(target_pair != NULL) {
		target_pair->value = value;
		return 0;		
	}

	// Item not found, add a new pair
	struct map_pair *new_pair = malloc(sizeof(struct map_pair));
	if(new_pair == NULL) {
		return -1;
	}

	new_pair->key = key;
	new_pair->value = value;
	LIST_INSERT_HEAD(&(m->pairs), new_pair, entries);

	m->size++;

	return 0;
}



void* map_get(map *m, long long key)
{
	struct map_pair *target_pair = find_pair_by_key(m, key);
	if(target_pair != NULL) {
		return target_pair->value;
	} else {
		return NULL;
	}
}



void* map_remove(map *m, long long key)
{
	struct map_pair *target_pair = find_pair_by_key(m, key);
	if(target_pair == NULL) {
		return NULL;
	}

	LIST_REMOVE(target_pair, entries);
	void *value = target_pair->value;
	free(target_pair);

	m->size--;

	return value;
}



int map_size(map *m)
{
	return m->size;
}



map_iterator* map_iterator_new(map *m)
{
	map_iterator *iterator = (map_iterator *) malloc(sizeof(map_iterator));
	if(iterator != NULL) {
		iterator->current = LIST_FIRST(&(m->pairs));
		iterator->previous = NULL;
		iterator->source = m;
	}
	return iterator;
}



void* map_iterator_next(map_iterator *iterator, long long *key)
{
	if(iterator->current == NULL) {
		return NULL;
	}

	if(key != NULL) {
		*key = iterator->current->key;
	}

	iterator->previous = iterator->current;
	iterator->current = LIST_NEXT(iterator->current, entries);

	return iterator->previous->value;
}



void map_iterator_remove(map_iterator *iterator)
{
	LIST_REMOVE(iterator->previous, entries);
	free(iterator->previous);
	iterator->source->size--;
}




void map_iterator_free(map_iterator *iterator)
{
	free(iterator);
}
