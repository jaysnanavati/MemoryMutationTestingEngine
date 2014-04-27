

/* This is auto-generated code. Edit at your own peril. */
#include <stdio.h>
#include "CuTest.h"


extern void TestWR_InitPool(CuTest*);
extern void TestWR_ObtainIncreasesCount(CuTest*);
extern void TestWR_GetGetsObject(CuTest*);
extern void TestWR_GetFailsOnNonExistantObject(CuTest*);
extern void TestWR_ReleaseLowersRefCountAndFailsGetObject(CuTest*);
extern void TestWR_RemovalAndGetLowersRefcountAndFailsGetObject(CuTest*);


void RunAllTests(void) 
{
    CuString *output = CuStringNew();
    CuSuite* suite = CuSuiteNew();


    SUITE_ADD_TEST(suite, TestWR_InitPool);
    SUITE_ADD_TEST(suite, TestWR_ObtainIncreasesCount);
    SUITE_ADD_TEST(suite, TestWR_GetGetsObject);
    SUITE_ADD_TEST(suite, TestWR_GetFailsOnNonExistantObject);
    SUITE_ADD_TEST(suite, TestWR_ReleaseLowersRefCountAndFailsGetObject);
    SUITE_ADD_TEST(suite, TestWR_RemovalAndGetLowersRefcountAndFailsGetObject);

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

