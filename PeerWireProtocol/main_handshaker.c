

/* This is auto-generated code. Edit at your own peril. */
#include <stdio.h>
#include "CuTest.h"


extern void TestPWP_handshake_disconnect_if_handshake_has_invalid_name_length(CuTest*);
extern void TestPWP_handshake_disconnect_if_handshake_has_invalid_protocol_name(CuTest*);
extern void TestPWP_handshake_disconnect_if_handshake_has_infohash_that_is_not_same_as_ours(CuTest*);
extern void TestPWP_handshake_success_from_good_handshake(CuTest*);


void RunAllTests(void) 
{
    CuString *output = CuStringNew();
    CuSuite* suite = CuSuiteNew();


    SUITE_ADD_TEST(suite, TestPWP_handshake_disconnect_if_handshake_has_invalid_name_length);
    SUITE_ADD_TEST(suite, TestPWP_handshake_disconnect_if_handshake_has_invalid_protocol_name);
    SUITE_ADD_TEST(suite, TestPWP_handshake_disconnect_if_handshake_has_infohash_that_is_not_same_as_ours);
    SUITE_ADD_TEST(suite, TestPWP_handshake_success_from_good_handshake);

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

