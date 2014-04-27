

/* This is auto-generated code. Edit at your own peril. */
#include <stdio.h>
#include "CuTest.h"


extern void TesthashmapqQuadratic_New(CuTest*);
extern void TesthashmapqQuadratic_Put(CuTest*);
extern void TesthashmapqQuadratic_PutEnsuresCapacity(CuTest*);
extern void TesthashmapqQuadratic_PutHandlesCollision(CuTest*);
extern void TesthashmapqQuadratic_GetHandlesCollisionByTraversingChain(CuTest*);
extern void TesthashmapqQuadratic_RemoveReturnsNullIfMissingAndTraversesChain(CuTest*);
extern void TesthashmapqQuadratic_RemoveHandlesCollision(CuTest*);
extern void TesthashmapqQuadratic_PutEntry(CuTest*);
extern void TesthashmapqQuadratic_Get(CuTest*);
extern void TesthashmapqQuadratic_ContainsKey(CuTest*);
extern void TesthashmapqQuadratic_DoublePut(CuTest*);
extern void TesthashmapqQuadratic_Get2(CuTest*);
extern void TesthashmapqQuadratic_IncreaseCapacityDoesNotBreakhashmapq(CuTest*);
extern void TesthashmapqQuadratic_Remove(CuTest*);
extern void TesthashmapqQuadratic_ClearRemovesAll(CuTest*);
extern void TesthashmapqQuadratic_ClearHandlesCollision(CuTest*);
extern void TesthashmapqQuadratic_DoesNotHaveNextForEmptyIterator(CuTest*);
extern void TesthashmapqQuadratic_RemoveItemDoesNotHaveNextForEmptyIterator(CuTest*);
extern void TesthashmapqQuadratic_Iterate(CuTest*);
extern void TesthashmapqQuadratic_IterateHandlesCollisions(CuTest*);
extern void TesthashmapqQuadratic_IterateAndRemoveDoesntBreakIteration(CuTest*);


void RunAllTests(void) 
{
    CuString *output = CuStringNew();
    CuSuite* suite = CuSuiteNew();


    SUITE_ADD_TEST(suite, TesthashmapqQuadratic_New);
    SUITE_ADD_TEST(suite, TesthashmapqQuadratic_Put);
    SUITE_ADD_TEST(suite, TesthashmapqQuadratic_PutEnsuresCapacity);
    SUITE_ADD_TEST(suite, TesthashmapqQuadratic_PutHandlesCollision);
    SUITE_ADD_TEST(suite, TesthashmapqQuadratic_GetHandlesCollisionByTraversingChain);
    SUITE_ADD_TEST(suite, TesthashmapqQuadratic_RemoveReturnsNullIfMissingAndTraversesChain);
    SUITE_ADD_TEST(suite, TesthashmapqQuadratic_RemoveHandlesCollision);
    SUITE_ADD_TEST(suite, TesthashmapqQuadratic_PutEntry);
    SUITE_ADD_TEST(suite, TesthashmapqQuadratic_Get);
    SUITE_ADD_TEST(suite, TesthashmapqQuadratic_ContainsKey);
    SUITE_ADD_TEST(suite, TesthashmapqQuadratic_DoublePut);
    SUITE_ADD_TEST(suite, TesthashmapqQuadratic_Get2);
    SUITE_ADD_TEST(suite, TesthashmapqQuadratic_IncreaseCapacityDoesNotBreakhashmapq);
    SUITE_ADD_TEST(suite, TesthashmapqQuadratic_Remove);
    SUITE_ADD_TEST(suite, TesthashmapqQuadratic_ClearRemovesAll);
    SUITE_ADD_TEST(suite, TesthashmapqQuadratic_ClearHandlesCollision);
    SUITE_ADD_TEST(suite, TesthashmapqQuadratic_DoesNotHaveNextForEmptyIterator);
    SUITE_ADD_TEST(suite, TesthashmapqQuadratic_RemoveItemDoesNotHaveNextForEmptyIterator);
    SUITE_ADD_TEST(suite, TesthashmapqQuadratic_Iterate);
    SUITE_ADD_TEST(suite, TesthashmapqQuadratic_IterateHandlesCollisions);
    SUITE_ADD_TEST(suite, TesthashmapqQuadratic_IterateAndRemoveDoesntBreakIteration);

    CuSuiteRun(suite);
    CuSuiteSummary(suite, output);
    CuSuiteDetails(suite, output);
    printf("%s\n", output->buffer);
}

int main()
{
    RunAllTests();
    return 0;
}

