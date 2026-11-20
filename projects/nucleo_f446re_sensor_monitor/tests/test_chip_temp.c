#include "unity.h"
#include "chip_temp.h"

#define TEST_CAL1 939
#define TEST_CAL2 1185

void setUp(void) {}
void tearDown(void) {}

void test_raw_equal_cal1_is_30c(void) {
	TEST_ASSERT_EQUAL_INT(300, chip_temp_from_raw(939, TEST_CAL1, TEST_CAL2));
}

void test_raw_equal_cal2_is_110c(void) {
	TEST_ASSERT_EQUAL_INT(1100, chip_temp_from_raw(1185, TEST_CAL1, TEST_CAL2));
}

void test_raw_below_cal1_is_below_30c(void) {
	TEST_ASSERT_EQUAL_INT(294, chip_temp_from_raw(937, TEST_CAL1, TEST_CAL2));
}

void test_raw_above_cal1(void) {
	TEST_ASSERT_EQUAL_INT(498, chip_temp_from_raw(1000, TEST_CAL1, TEST_CAL2));
}

void test_raw_zero_does_not_overflow(void) {
	TEST_ASSERT_EQUAL_INT(-2753, chip_temp_from_raw(0, TEST_CAL1, TEST_CAL2));
}

void test_raw_max_does_not_overflow(void) {
	TEST_ASSERT_EQUAL_INT(10563, chip_temp_from_raw(4095, TEST_CAL1, TEST_CAL2));
}

void test_equal_cal_returns_zero(void) {
	TEST_ASSERT_EQUAL_INT(0, chip_temp_from_raw(1000, TEST_CAL1, TEST_CAL1));
}

int main(void) {
	UNITY_BEGIN();

	RUN_TEST(test_raw_equal_cal1_is_30c);
	RUN_TEST(test_raw_equal_cal2_is_110c);
	RUN_TEST(test_raw_below_cal1_is_below_30c);
	RUN_TEST(test_raw_above_cal1);
	RUN_TEST(test_raw_zero_does_not_overflow);
	RUN_TEST(test_raw_max_does_not_overflow);
	RUN_TEST(test_equal_cal_returns_zero);

	return UNITY_END();
}
