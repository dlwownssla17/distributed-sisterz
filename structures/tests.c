#include <stdio.h>
#include "map.h"

void assert(int should_be_true, char *message)
{
	if(!should_be_true) {
		printf("Map test error: %s\n", message);
	}
}

void test_new_free()
{
	map *m = map_new();
	assert(m != NULL, "map_new shouldn't be NULL");
	map_free(m);
}

void test_get_fails()
{
	map *m = map_new();
	void *result = map_get(m, 1);
	assert(result == NULL, "get on a missing key should return NULL");
	map_free(m);
}

void test_remove_fails()
{
	map *m = map_new();
	void *result = map_remove(m, 1);
	assert(result == NULL, "remove on a missing key should return NULL");
	assert(map_size(m) == 0, "remove on a missing key shouldn't change the map size");
	map_free(m);
}

void test_put_and_get()
{
	void *payload = (void *) 7;

	map *m = map_new();

	assert(map_size(m) == 0, "a new map should have size 0");

	int put_result = map_put(m, 1, payload);
	assert(put_result == 0, "put should always succeed");

	void *result = map_get(m, 1);
	assert(result == payload, "get should return what put inserted");

	assert(map_size(m) == 1, "adding an element should increase the size");

	map_remove(m, 1);
	map_free(m);
}

void test_put_and_remove()
{
	void *payload = (void *) 7;

	map *m = map_new();

	int put_result = map_put(m, 1, payload);
	assert(put_result == 0, "put should always succeed (tpr)");

	void *result = map_remove(m, 1);
	assert(result == payload, "remove should return what put inserted");

	assert(map_size(m) == 0, "removing an element should decrease the size");

	map_free(m);	
}

void test_put_updates()
{
	void *payload = (void *) 7;
	void *payload2 = (void *) 8;

	map *m = map_new();
	map_put(m, 1, payload);
	map_put(m, 1, payload2);

	assert(map_size(m) == 1, "updating an element shouldn't change the size");

	void *result = map_remove(m, 1);
	assert(result == payload2, "remove should return what put inserted (tpu)");

	map_free(m);	
}

void test_complex()
{
	void *payload = (void *) 7;
	void *payload2 = (void *) 8;
	void *payload3 = (void *) 9;

	map *m = map_new();

	int put_result = map_put(m, 1, payload);
	assert(put_result == 0, "put should always succeed (tc1)");

	put_result = map_put(m, 2, payload2);
	assert(put_result == 0, "put should always succeed (tc2)");

	void *result = map_remove(m, 1);
	assert(result == payload, "remove should return what put inserted (tc1)");

	put_result = map_put(m, 3, payload3);
	assert(put_result == 0, "put should always succeed (tc3)");

	result = map_remove(m, 2);
	assert(result == payload2, "remove should return what put inserted (tc2)");

	result = map_get(m, 2);
	assert(result == NULL, "remove on a missing key should return NULL (tc)");

	result = map_get(m, 3);
	assert(result == payload3, "get should return what put inserted (tc)");

	result = map_remove(m, 3);
	assert(result == payload3, "remove should return what put inserted (tc3)");

	map_free(m);
}

void test_iterators()
{
	void *payload = (void *) 7;
	void *payload2 = (void *) 8;
	void *payload3 = (void *) 9;

	map *m = map_new();

	map_put(m, 3, payload3);
	map_put(m, 2, payload2);
	map_put(m, 1, payload);

	// This test currently hardcodes the order, but this shouldn't be guaranteed.
	map_iterator *iterator = map_iterator_new(m);
	long long current_key;
	void *current_value = map_iterator_next(iterator, &current_key);
	assert(current_value == payload, "first iterator return should be first map value");
	assert(current_key == 1, "first iterator key should be correct");

	current_value = map_iterator_next(iterator, NULL);
	assert(current_value == payload2, "second iterator return should be second map value");

	current_value = map_iterator_next(iterator, &current_key);
	assert(current_value == payload3, "third iterator return should be third map value");
	assert(current_key == 3, "third iterator key should be correct");

	current_value = map_iterator_next(iterator, &current_key);
	assert(current_value == NULL, "fourth iterator return should be NULL");

	map_iterator_free(iterator);
	map_free(m);
}

void test_iterator_empty()
{
	map *m = map_new();
	map_iterator *iterator = map_iterator_new(m);
	void *current_value = map_iterator_next(iterator, NULL);
	assert(current_value == NULL, "empty iterator immediately returns NULL");

	map_iterator_free(iterator);
	map_free(m);
}

void test_iterator_remove()
{
	void *payload = (void *) 7;
	void *payload2 = (void *) 8;
	void *payload3 = (void *) 9;

	map *m = map_new();

	map_put(m, 3, payload3);
	map_put(m, 2, payload2);
	map_put(m, 1, payload);

	// This test currently hardcodes the order, but this shouldn't be guaranteed.
	map_iterator *iterator = map_iterator_new(m);
	map_iterator_next(iterator, NULL);
	map_iterator_next(iterator, NULL);
	map_iterator_remove(iterator);
	void *current_value = map_iterator_next(iterator, NULL);
	assert(current_value == payload3, "removing an iterator element should not disrupt its continuation");
	map_iterator_free(iterator);

	iterator = map_iterator_new(m);
	current_value = map_iterator_next(iterator, NULL);
	assert(current_value == payload, "iterator removal verification 1");
	current_value = map_iterator_next(iterator, NULL);
	assert(current_value == payload3, "iterator removal verification 2");
	current_value = map_iterator_next(iterator, NULL);
	assert(current_value == NULL, "iterator removal verification 3");
	assert(map_size(m) == 2, "iterator removal should affect size");
	map_iterator_free(iterator);

	map_free(m);
}

int main()
{
	test_new_free();
	test_get_fails();
	test_remove_fails();
	test_put_and_get();
	test_put_and_remove();
	test_put_updates();
	test_complex();
	test_iterators();
	test_iterator_empty();
	test_iterator_remove();
}
