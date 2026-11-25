#include "unity.h"
#include "mcp9808_conv.h"

void setUp(void) {}
void tearDown(void) {}

void test_positive_25c(void) {
	TEST_ASSERT_EQUAL_INT(250, mcp9808_conv_to_x10(0x0190));
}

void test_zero(void) {
	TEST_ASSERT_EQUAL_INT(0, mcp9808_conv_to_x10(0x0000));
}

void test_negative_1c(void) {
	TEST_ASSERT_EQUAL_INT(-10, mcp9808_conv_to_x10(0x1FF0));
}

void test_alert_flags_are_ignored(void) {
	TEST_ASSERT_EQUAL_INT(250, mcp9808_conv_to_x10(0xE190));
}

void test_fraction_truncates(void) {
	TEST_ASSERT_EQUAL_INT(250, mcp9808_conv_to_x10(0x0191));
}

void test_max_positive(void) {
	TEST_ASSERT_EQUAL_INT(2559, mcp9808_conv_to_x10(0x0FFF));
}

void test_min_negative(void) {
	TEST_ASSERT_EQUAL_INT(-2560, mcp9808_conv_to_x10(0x1000));
}


int main(void) {
	UNITY_BEGIN();

	RUN_TEST(test_positive_25c);
	RUN_TEST(test_zero);
	RUN_TEST(test_negative_1c);
	RUN_TEST(test_alert_flags_are_ignored);
	RUN_TEST(test_fraction_truncates);
	RUN_TEST(test_max_positive);
	RUN_TEST(test_min_negative);

	return UNITY_END();
}
