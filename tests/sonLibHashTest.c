#include "sonLibGlobalsTest.h"

static stHash *hash;
static stHash *hash2;
static stIntTuple *one, *two, *three, *four, *five, *six;

static void testSetup() {
    //compare by value of memory address
    hash = stHash_construct();
    //compare by value of ints.
    hash2 = stHash_construct3((uint32_t (*)(const void *))stIntTuple_hashKey,
            (int (*)(const void *, const void *))stIntTuple_equalsFn,
            (void (*)(void *))stIntTuple_destruct,
            (void (*)(void *))stIntTuple_destruct);
    one = stIntTuple_construct(1, 0);
    two = stIntTuple_construct(1, 1);
    three = stIntTuple_construct(1, 2);
    four = stIntTuple_construct(1, 3);
    five = stIntTuple_construct(1, 4);
    six = stIntTuple_construct(1, 5);

    stHash_insert(hash, one, two);
    stHash_insert(hash, three, four);
    stHash_insert(hash, five, six);

    stHash_insert(hash2, one, two);
    stHash_insert(hash2, three, four);
    stHash_insert(hash2, five, six);
}

static void testTeardown() {
    stHash_destruct(hash);
    stHash_destruct(hash2);
}

static void testHash_construct(CuTest* testCase) {
    assert(testCase != NULL);
    testSetup();
    /* Do nothing */
    testTeardown();
}

static void testHash_search(CuTest* testCase) {
    testSetup();

    stIntTuple *i = stIntTuple_construct(1, 0);

    //Check search by memory address
    CuAssertTrue(testCase, stHash_search(hash, one) == two);
    CuAssertTrue(testCase, stHash_search(hash, three) == four);
    CuAssertTrue(testCase, stHash_search(hash, five) == six);
    //Check not present
    CuAssertTrue(testCase, stHash_search(hash, six) == NULL);
    CuAssertTrue(testCase, stHash_search(hash, i) == NULL);

    //Check search by memory address
    CuAssertTrue(testCase, stHash_search(hash2, one) == two);
    CuAssertTrue(testCase, stHash_search(hash2, three) == four);
    CuAssertTrue(testCase, stHash_search(hash2, five) == six);
    //Check not present
    CuAssertTrue(testCase, stHash_search(hash2, six) == NULL);
    //Check is searching by memory.
    CuAssertTrue(testCase, stHash_search(hash2, i) == two);

    stIntTuple_destruct(i);

    testTeardown();
}

static void testHash_remove(CuTest* testCase) {
    testSetup();

    CuAssertTrue(testCase, stHash_remove(hash, one) == two);
    CuAssertTrue(testCase, stHash_search(hash, one) == NULL);

    CuAssertTrue(testCase, stHash_remove(hash2, one) == two);
    CuAssertTrue(testCase, stHash_search(hash2, one) == NULL);

    stHash_insert(hash2, one, two);
    CuAssertTrue(testCase, stHash_search(hash2, one) == two);

    testTeardown();
}

static void testHash_insert(CuTest* testCase) {
    /*
     * Tests inserting already present keys.
     */
    testSetup();

    CuAssertTrue(testCase, stHash_search(hash, one) == two);
    stHash_insert(hash, one, two);
    CuAssertTrue(testCase, stHash_search(hash, one) == two);
    stHash_insert(hash, one, three);
    CuAssertTrue(testCase, stHash_search(hash, one) == three);
    stHash_insert(hash, one, two);
    CuAssertTrue(testCase, stHash_search(hash, one) == two);

    testTeardown();
}

static void testHash_size(CuTest *testCase) {
    /*
     * Tests the size function of the hash.
     */
    testSetup();

    CuAssertTrue(testCase, stHash_size(hash) == 3);
    CuAssertTrue(testCase, stHash_size(hash2) == 3);
    stHash *hash3 = stHash_construct();
    CuAssertTrue(testCase, stHash_size(hash3) == 0);
    stHash_destruct(hash3);

    testTeardown();
}

static void testHash_testIterator(CuTest *testCase) {
    testSetup();

    stHashIterator *iterator = stHash_getIterator(hash);
    stHashIterator *iteratorCopy = stHash_copyIterator(iterator);
    int32_t i=0;
    stHash *seen = stHash_construct();
    for(i=0; i<3; i++) {
        void *o = stHash_getNext(iterator);
        CuAssertTrue(testCase, o != NULL);
        CuAssertTrue(testCase, stHash_search(hash, o) != NULL);
        CuAssertTrue(testCase, stHash_search(seen, o) == NULL);
        CuAssertTrue(testCase, stHash_getNext(iteratorCopy) == o);
        stHash_insert(seen, o, o);
    }
    CuAssertTrue(testCase, stHash_getNext(iterator) == NULL);
    CuAssertTrue(testCase, stHash_getNext(iterator) == NULL);
    CuAssertTrue(testCase, stHash_getNext(iteratorCopy) == NULL);
    stHash_destruct(seen);
    stHash_destructIterator(iterator);
    stHash_destructIterator(iteratorCopy);

    testTeardown();
}

static void testHash_testGetKeys(CuTest *testCase) {
    testSetup();
    stList *list = stHash_getKeys(hash2);
    CuAssertTrue(testCase, stList_length(list) == 3);
    CuAssertTrue(testCase, stList_contains(list, one));
    CuAssertTrue(testCase, stList_contains(list, three));
    CuAssertTrue(testCase, stList_contains(list, five));
    stList_destruct(list);
    testTeardown();
}

static void testHash_testGetValues(CuTest *testCase) {
    testSetup();
    stList *list = stHash_getValues(hash2);
    CuAssertTrue(testCase, stList_length(list) == 3);
    CuAssertTrue(testCase, stList_contains(list, two));
    CuAssertTrue(testCase, stList_contains(list, four));
    CuAssertTrue(testCase, stList_contains(list, six));
    stList_destruct(list);
    testTeardown();
}

CuSuite* sonLibHashTestSuite(void) {
    CuSuite* suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, testHash_search);
    SUITE_ADD_TEST(suite, testHash_remove);
    SUITE_ADD_TEST(suite, testHash_insert);
    SUITE_ADD_TEST(suite, testHash_size);
    SUITE_ADD_TEST(suite, testHash_testIterator);
    SUITE_ADD_TEST(suite, testHash_construct);
    SUITE_ADD_TEST(suite, testHash_testGetKeys);
    SUITE_ADD_TEST(suite, testHash_testGetValues);
    return suite;
}
