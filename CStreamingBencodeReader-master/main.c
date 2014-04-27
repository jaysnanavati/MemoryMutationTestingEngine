

/* This is auto-generated code. Edit at your own peril. */
#include <stdio.h>
#include "CuTest.h"


extern void TestBencodeTest_add_sibling_adds_sibling(CuTest*);
extern void TestBencodeTest_add_child_adds_child(CuTest*);
extern void TestBencode_fail_if_depth_not_sufficient(CuTest*);
extern void TestBencodeIntValue(CuTest*);
extern void TestBencodeIntValue2(CuTest*);
extern void TestBencodeIntValueLarge(CuTest*);
extern void TestBencodeIsIntEmpty(CuTest*);
extern void TestBencodeStringValue(CuTest*);
extern void TestBencodeStringValue2(CuTest*);
extern void TestBencodeStringHandlesNonAscii0(CuTest*);
extern void TestBencodeStringValueWithColon(CuTest*);
extern void TestBencodeIsList(CuTest*);
extern void TestBencodeListGetNext(CuTest*);
extern void TestBencodeListInListWithValue(CuTest*);
extern void TestBencodeListDoesntHasNextWhenEmpty(CuTest*);
extern void TestBencodeEmptyListInListWontGetNextIfEmpty(CuTest*);
extern void TestBencodeEmptyListInListWontGetNextTwiceIfEmpty(CuTest*);
extern void TestBencodeListGetNextTwiceWhereOnlyOneAvailable(CuTest*);
extern void TestBencodeListGetNextTwice(CuTest*);
extern void TestBencodeDictGetNext(CuTest*);
extern void TestBencodeDictWontGetNextIfEmpty(CuTest*);
extern void TestBencodeDictGetNextTwice(CuTest*);
extern void TestBencodeDictGetNextTwiceOnlyIfSecondKeyValid(CuTest*);
extern void TestBencodeDictGetNextInnerList(CuTest*);
extern void TestBencodeDictInnerList(CuTest*);
extern void TestBencodeStringValueIsZeroLength(CuTest*);
extern void TestBencodeListNextGetsCalled(CuTest*);
extern void TestBencodeDictNextGetsCalled(CuTest*);
extern void TestBencodeListLeaveGetsCalled(CuTest*);
extern void TestBencodeDictLeaveGetsCalled(CuTest*);


void RunAllTests(void) 
{
    CuString *output = CuStringNew();
    CuSuite* suite = CuSuiteNew();


    SUITE_ADD_TEST(suite, TestBencodeTest_add_sibling_adds_sibling);
    SUITE_ADD_TEST(suite, TestBencodeTest_add_child_adds_child);
    SUITE_ADD_TEST(suite, TestBencode_fail_if_depth_not_sufficient);
    SUITE_ADD_TEST(suite, TestBencodeIntValue);
    SUITE_ADD_TEST(suite, TestBencodeIntValue2);
    SUITE_ADD_TEST(suite, TestBencodeIntValueLarge);
    SUITE_ADD_TEST(suite, TestBencodeIsIntEmpty);
    SUITE_ADD_TEST(suite, TestBencodeStringValue);
    SUITE_ADD_TEST(suite, TestBencodeStringValue2);
    SUITE_ADD_TEST(suite, TestBencodeStringHandlesNonAscii0);
    SUITE_ADD_TEST(suite, TestBencodeStringValueWithColon);
    SUITE_ADD_TEST(suite, TestBencodeIsList);
    SUITE_ADD_TEST(suite, TestBencodeListGetNext);
    SUITE_ADD_TEST(suite, TestBencodeListInListWithValue);
    SUITE_ADD_TEST(suite, TestBencodeListDoesntHasNextWhenEmpty);
    SUITE_ADD_TEST(suite, TestBencodeEmptyListInListWontGetNextIfEmpty);
    SUITE_ADD_TEST(suite, TestBencodeEmptyListInListWontGetNextTwiceIfEmpty);
    SUITE_ADD_TEST(suite, TestBencodeListGetNextTwiceWhereOnlyOneAvailable);
    SUITE_ADD_TEST(suite, TestBencodeListGetNextTwice);
    SUITE_ADD_TEST(suite, TestBencodeDictGetNext);
    SUITE_ADD_TEST(suite, TestBencodeDictWontGetNextIfEmpty);
    SUITE_ADD_TEST(suite, TestBencodeDictGetNextTwice);
    SUITE_ADD_TEST(suite, TestBencodeDictGetNextTwiceOnlyIfSecondKeyValid);
    SUITE_ADD_TEST(suite, TestBencodeDictGetNextInnerList);
    SUITE_ADD_TEST(suite, TestBencodeDictInnerList);
    SUITE_ADD_TEST(suite, TestBencodeStringValueIsZeroLength);
    SUITE_ADD_TEST(suite, TestBencodeListNextGetsCalled);
    SUITE_ADD_TEST(suite, TestBencodeDictNextGetsCalled);
    SUITE_ADD_TEST(suite, TestBencodeListLeaveGetsCalled);
    SUITE_ADD_TEST(suite, TestBencodeDictLeaveGetsCalled);

    CuSuiteRun(suite);
    CuSuiteDetails(suite, output);
    printf("%s\n", output->buffer);
}

int main()
{
    RunAllTests();
    return 0;
}

