

/* This is auto-generated code. Edit at your own peril. */
#include <stdio.h>
#include "CuTest.h"


extern void TestPWP_keepalive(CuTest*);
extern void TestPWP_choke(CuTest*);
extern void TestPWP_unchoke(CuTest*);
extern void TestPWP_interested(CuTest*);
extern void TestPWP_uninterested(CuTest*);
extern void TestPWP_have(CuTest*);
extern void TestPWP_request(CuTest*);
extern void TestPWP_cancel(CuTest*);
extern void TestPWP_bitfield(CuTest*);
extern void TestPWP_piece(CuTest*);
extern void TestPWP_piece_halfread(CuTest*);
extern void TestPWP_two_pieces(CuTest*);
extern void TestPWP_custom_handler(CuTest*);


void RunAllTests(void) 
{
    CuString *output = CuStringNew();
    CuSuite* suite = CuSuiteNew();


    SUITE_ADD_TEST(suite, TestPWP_keepalive);
    SUITE_ADD_TEST(suite, TestPWP_choke);
    SUITE_ADD_TEST(suite, TestPWP_unchoke);
    SUITE_ADD_TEST(suite, TestPWP_interested);
    SUITE_ADD_TEST(suite, TestPWP_uninterested);
    SUITE_ADD_TEST(suite, TestPWP_have);
    SUITE_ADD_TEST(suite, TestPWP_request);
    SUITE_ADD_TEST(suite, TestPWP_cancel);
    SUITE_ADD_TEST(suite, TestPWP_bitfield);
    SUITE_ADD_TEST(suite, TestPWP_piece);
    SUITE_ADD_TEST(suite, TestPWP_piece_halfread);
    SUITE_ADD_TEST(suite, TestPWP_two_pieces);
    SUITE_ADD_TEST(suite, TestPWP_custom_handler);

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

