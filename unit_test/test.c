#include "/home/magno/Unity/src/unity.h"
#include "calc_time.h"
#include "time.h"

void setUp(void){}
void tearDown(void){
	struct tm timeinfo = {0,};
}

void beginning_hour(void){
	struct tm timeinfo = {.tm_min = 0,.tm_sec = 0};

	TEST_ASSERT_EQUAL(0,next_sleep_time(timeinfo,-2));
	TEST_ASSERT_EQUAL(0,next_sleep_time(timeinfo,0));
	TEST_ASSERT_EQUAL(120,next_sleep_time(timeinfo,2));
	TEST_ASSERT_EQUAL(180,next_sleep_time(timeinfo,3));
	TEST_ASSERT_EQUAL(300,next_sleep_time(timeinfo,5));
	TEST_ASSERT_EQUAL(600,next_sleep_time(timeinfo,10));
	TEST_ASSERT_EQUAL(1200,next_sleep_time(timeinfo,20));
	TEST_ASSERT_EQUAL(1800,next_sleep_time(timeinfo,30));
}

void early_hour(void){
	struct tm timeinfo = {.tm_min = 2,.tm_sec = 41};

	TEST_ASSERT_EQUAL(0,next_sleep_time(timeinfo,-2));
	TEST_ASSERT_EQUAL(0,next_sleep_time(timeinfo,0));
	TEST_ASSERT_EQUAL(79,next_sleep_time(timeinfo,2));
	TEST_ASSERT_EQUAL(19,next_sleep_time(timeinfo,3));
	TEST_ASSERT_EQUAL(139,next_sleep_time(timeinfo,5));
	TEST_ASSERT_EQUAL(439,next_sleep_time(timeinfo,10));
	TEST_ASSERT_EQUAL(1039,next_sleep_time(timeinfo,20));
	TEST_ASSERT_EQUAL(1639,next_sleep_time(timeinfo,30));
}

void almost_half_hour(void){
	struct tm timeinfo = {.tm_min = 28, .tm_sec = 15};

	TEST_ASSERT_EQUAL(0,next_sleep_time(timeinfo,-2));
	TEST_ASSERT_EQUAL(0,next_sleep_time(timeinfo,0));
	TEST_ASSERT_EQUAL(105,next_sleep_time(timeinfo,2));
	TEST_ASSERT_EQUAL(105,next_sleep_time(timeinfo,3));
	TEST_ASSERT_EQUAL(105,next_sleep_time(timeinfo,5));
	TEST_ASSERT_EQUAL(105,next_sleep_time(timeinfo,10));
	TEST_ASSERT_EQUAL(705,next_sleep_time(timeinfo,20));
	TEST_ASSERT_EQUAL(105,next_sleep_time(timeinfo,30));	
} 

void past_half_hour(void){
	struct tm timeinfo = {.tm_min = 33, .tm_sec = 27};

	TEST_ASSERT_EQUAL(0,next_sleep_time(timeinfo,-2));
	TEST_ASSERT_EQUAL(0,next_sleep_time(timeinfo,0));
	TEST_ASSERT_EQUAL(33,next_sleep_time(timeinfo,2));
	TEST_ASSERT_EQUAL(153,next_sleep_time(timeinfo,3));
	TEST_ASSERT_EQUAL(93,next_sleep_time(timeinfo,5));
	TEST_ASSERT_EQUAL(393,next_sleep_time(timeinfo,10));
	TEST_ASSERT_EQUAL(393,next_sleep_time(timeinfo,20));
	TEST_ASSERT_EQUAL(1593,next_sleep_time(timeinfo,30));	
}

void edge_minutes_to_sharp_hour(void){
	struct tm timeinfo = {.tm_min = 57, .tm_sec = 32};

	TEST_ASSERT_EQUAL(0,next_sleep_time(timeinfo,-2));
	TEST_ASSERT_EQUAL(0,next_sleep_time(timeinfo,0));
	TEST_ASSERT_EQUAL(28,next_sleep_time(timeinfo,2));
	TEST_ASSERT_EQUAL(148,next_sleep_time(timeinfo,3));
	TEST_ASSERT_EQUAL(148,next_sleep_time(timeinfo,5));
	TEST_ASSERT_EQUAL(148,next_sleep_time(timeinfo,10));
	TEST_ASSERT_EQUAL(148,next_sleep_time(timeinfo,20));
	TEST_ASSERT_EQUAL(148,next_sleep_time(timeinfo,30));
}


int main(){
	UNITY_BEGIN();
	RUN_TEST(beginning_hour);
	RUN_TEST(early_hour);
	RUN_TEST(almost_half_hour);
	RUN_TEST(past_half_hour);
	RUN_TEST(edge_minutes_to_sharp_hour);
	return UNITY_END();
}

// gcc test.c calc_time.c /home/magno/Unity/src/unity.c -o test