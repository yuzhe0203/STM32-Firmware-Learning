#include "unity.h"
#include "temp_zone.h"

void setUp(void) {}
void tearDown(void) {}

void test_none_uses_plain_thresholds(void) {
	TEST_ASSERT_EQUAL_INT(TEMP_ZONE_BLUE, temp_zone_next(TEMP_ZONE_NONE, 255));
	TEST_ASSERT_EQUAL_INT(TEMP_ZONE_GREEN, temp_zone_next(TEMP_ZONE_NONE, 260));
	TEST_ASSERT_EQUAL_INT(TEMP_ZONE_RED, temp_zone_next(TEMP_ZONE_NONE, 300));
}

void test_blue_rises_to_green_at_threshold(void) {
	TEST_ASSERT_EQUAL_INT(TEMP_ZONE_BLUE, temp_zone_next(TEMP_ZONE_BLUE, 259));
	TEST_ASSERT_EQUAL_INT(TEMP_ZONE_GREEN, temp_zone_next(TEMP_ZONE_BLUE, 260));
}

void test_green_holds_inside_hysteresis(void) {
	TEST_ASSERT_EQUAL_INT(TEMP_ZONE_GREEN, temp_zone_next(TEMP_ZONE_GREEN, 258));
	TEST_ASSERT_EQUAL_INT(TEMP_ZONE_GREEN, temp_zone_next(TEMP_ZONE_GREEN, 257));
}

void test_green_drops_to_blue_below_hysteresis(void) {
	TEST_ASSERT_EQUAL_INT(TEMP_ZONE_BLUE, temp_zone_next(TEMP_ZONE_GREEN, 256));
}

void test_red_holds_inside_hysteresis(void) {
	TEST_ASSERT_EQUAL_INT(TEMP_ZONE_RED, temp_zone_next(TEMP_ZONE_RED, 297));
}

void test_red_drops_to_green_below_hysteresis(void) {
	TEST_ASSERT_EQUAL_INT(TEMP_ZONE_GREEN, temp_zone_next(TEMP_ZONE_RED, 296));
}

void test_red_drops_directly_to_blue(void) {
	TEST_ASSERT_EQUAL_INT(TEMP_ZONE_BLUE, temp_zone_next(TEMP_ZONE_RED, 250));
}

void test_negative_temperature_is_blue(void) {
	TEST_ASSERT_EQUAL_INT(TEMP_ZONE_BLUE, temp_zone_next(TEMP_ZONE_NONE, -5));
	TEST_ASSERT_EQUAL_INT(TEMP_ZONE_BLUE, temp_zone_next(TEMP_ZONE_GREEN, -5));
	TEST_ASSERT_EQUAL_INT(TEMP_ZONE_BLUE, temp_zone_next(TEMP_ZONE_RED, -5));
}

void test_zone_to_string(void) {
	TEST_ASSERT_EQUAL_STRING("None", temp_zone_to_string(TEMP_ZONE_NONE));
	TEST_ASSERT_EQUAL_STRING("Blue", temp_zone_to_string(TEMP_ZONE_BLUE));
	TEST_ASSERT_EQUAL_STRING("Green", temp_zone_to_string(TEMP_ZONE_GREEN));
	TEST_ASSERT_EQUAL_STRING("Red", temp_zone_to_string(TEMP_ZONE_RED));
	TEST_ASSERT_EQUAL_STRING("Unknown", temp_zone_to_string((temp_zone_t)99));
}

int main(void) {
	UNITY_BEGIN();

	RUN_TEST(test_none_uses_plain_thresholds);
	RUN_TEST(test_blue_rises_to_green_at_threshold);
	RUN_TEST(test_green_holds_inside_hysteresis);
	RUN_TEST(test_green_drops_to_blue_below_hysteresis);
	RUN_TEST(test_red_holds_inside_hysteresis);
	RUN_TEST(test_red_drops_to_green_below_hysteresis);
	RUN_TEST(test_red_drops_directly_to_blue);
	RUN_TEST(test_negative_temperature_is_blue);
	RUN_TEST(test_zone_to_string);

	return UNITY_END();
}
