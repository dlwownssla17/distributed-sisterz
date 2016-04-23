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

int main()
{
	test_new_free();
	test_get_fails();
	test_remove_fails();
	test_put_and_get();
	test_put_and_remove();
	test_put_updates();
	test_complex();
}
