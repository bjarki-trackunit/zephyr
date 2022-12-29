/*
 * Copyright 2022 Bjarki Arge Andreasen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/drivers/rtcc.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/timeutil.h>
#include <zephyr/ztest.h>

/* Tue Dec 31 2024 23:59:50 GMT+0000 */
#define RTCC_TEST_DEFAULT_TIME          (1735689590L)

/*
 * Alarm is 10 seconds after default time, on the minute
 * Wed Jan 1 2025 00:00:00 GMT+0000
 * Note weekday is in days since monday like the tm struct
 */
#define RTCC_TEST_ALARM_MIN             (0)
#define RTCC_TEST_ALARM_HOUR            (0)
#define RTCC_TEST_ALARM_WEEKDAY         (3)

/* Wed Dec 31 2025 23:59:50 GMT+0000 */
#define RTCC_TEST_GET_SET_TIME          (1767225590L)
#define RTCC_TEST_GET_SET_TIME_TOL      (1L)

struct rtcc_test_fixture {
	/* Device and devicetree config */
	const struct device *rtcc;
	const size_t rtcc_alarms_count;
	const size_t rtcc_timestamps_count;

	/* Alarm test */
	atomic_t alarms_triggered;
	int32_t alarms_user_data;
};

static struct rtcc_test_fixture fixture_instance = {
	.rtcc = DEVICE_DT_GET(DT_ALIAS(rtcc)),
	.rtcc_alarms_count = DT_PROP(DT_ALIAS(rtcc), alarms_count),
	.rtcc_timestamps_count = DT_PROP(DT_ALIAS(rtcc), timestamps_count),
};

ZTEST(rtcc_user, test_time_set_get)
{
	struct tm datetime_set;
	struct tm datetime_get;
	time_t timer_get;
	time_t timer_set = RTCC_TEST_GET_SET_TIME;

	gmtime_r(&timer_set, &datetime_set);

	zassert_true(rtcc_time_set(fixture_instance.rtcc, &datetime_set) == 0,
		     "Failed to set time");

	zassert_true(rtcc_time_get(fixture_instance.rtcc, &datetime_get) == 0,
		     "Failed to get time using rtcc_time_get()");

	timer_get = timeutil_timegm(&datetime_get);

	zassert_true((timer_get >= RTCC_TEST_GET_SET_TIME) &&
		     (timer_get <= (RTCC_TEST_GET_SET_TIME + RTCC_TEST_GET_SET_TIME_TOL)),
		     "Got unexpected time");
}

ZTEST(rtcc_user, test_alarms_not_sup)
{
	/* Skip test if alarms are supported by hardware */
	if (fixture_instance.rtcc_alarms_count > 0) {
		return;
	}

	zassert_true(rtcc_alarm_config_set(fixture_instance.rtcc, 0, NULL) == -ENOTSUP,
		     "Should return -ENOTSUP if not supported by hardware");

	zassert_true(rtcc_alarm_state_get(fixture_instance.rtcc, 0, NULL) == -ENOTSUP,
		     "Should return -ENOTSUP if not supported by hardware");

	zassert_true(rtcc_alarm_triggered_test(fixture_instance.rtcc, 0) == -ENOTSUP,
		     "Should return -ENOTSUP if not supported by hardware");

	zassert_true(rtcc_alarm_triggered_clear(fixture_instance.rtcc, 0) == -ENOTSUP,
		     "Should return -ENOTSUP if not supported by hardware");
}

/* Validate that the RTCC alarms work as intended */
static void rtcc_user_alarm_handler(const struct device *dev, size_t id, void *user_data)
{
	zassert_true(dev == fixture_instance.rtcc,
		     "Unexpected device passed to alarm handler");

	zassert_true(user_data == &fixture_instance.alarms_user_data,
		     "Unexpected user_data passed to alarm handler");

	atomic_set_bit(&fixture_instance.alarms_triggered, id);
}

ZTEST(rtcc_user, test_alarms)
{
	struct tm pre_alarm_datetime;
	struct rtcc_alarm_state state;
	time_t pre_alarm_timer = RTCC_TEST_DEFAULT_TIME;

	zassume_true(fixture_instance.rtcc_alarms_count > 0,
		     "RTCC does not support alarms");

	gmtime_r(&pre_alarm_timer, &pre_alarm_datetime);

	/* Alarm match values (10 seconds after pre_alarm_datetime) */
	struct tm alarm_match_datetime = {
		.tm_min = RTCC_TEST_ALARM_MIN,
		.tm_hour = RTCC_TEST_ALARM_HOUR,
		.tm_wday = RTCC_TEST_ALARM_WEEKDAY,
	};

	/* Alarm match mask */
	uint32_t alarm_match_mask = (RTCC_ALARM_MATCH_MASK_MINUTE |
				     RTCC_ALARM_MATCH_MASK_HOUR |
				     RTCC_ALARM_MATCH_MASK_WEEKDAY);

	struct rtcc_alarm_config config = {
		.datetime = &alarm_match_datetime,
		.mask = alarm_match_mask,
		.handler = rtcc_user_alarm_handler,
		.user_data = &fixture_instance.alarms_user_data,
	};

	/*
	 * Configure alarms and verify state
	 */

	/* Configure all alarms */
	for (size_t i = 0; i < fixture_instance.rtcc_alarms_count; i++) {
		zassert_true(rtcc_alarm_config_set(fixture_instance.rtcc, i, &config) == 0,
			     "Failed to configure alarm");
	}

	/* Validate all alarm states */
	for (size_t i = 0; i < fixture_instance.rtcc_alarms_count; i++) {
		zassert_true(rtcc_alarm_state_get(fixture_instance.rtcc, i, &state) == 0,
			     "Failed to get alarm state");

		zassert_true((state.datetime.tm_min == alarm_match_datetime.tm_min) &&
			     (state.datetime.tm_hour == alarm_match_datetime.tm_hour) &&
			     (state.datetime.tm_wday == alarm_match_datetime.tm_wday) &&
			     (state.mask == alarm_match_mask),
			     "state does not match condiguration");

		zassert_true(state.enabled == false,
			     "Alarm must not be auto enabled from config set");

		zassert_true(state.triggered == false,
			     "Alarm triggered must be reset when config is set");
	}

	/* Validate alarms not reporting as triggered */
	for (size_t i = 0; i < fixture_instance.rtcc_alarms_count; i++) {
		zassert_true(rtcc_alarm_triggered_test(fixture_instance.rtcc, i) == 0,
			     "Alarm not reporting as not triggered");
	}

	/*
	 * Set time to 10 seconds before alarms shall trigger and wait for alarms to
	 * trigger
	 */

	/* Set RTCC time */
	zassert_true(rtcc_time_set(fixture_instance.rtcc, &pre_alarm_datetime) == 0,
		     "Failed to set time");

	/* Enable all alarms */
	for (size_t i = 0; i < fixture_instance.rtcc_alarms_count; i++) {
		zassert_true(rtcc_alarm_enable(fixture_instance.rtcc, i) == 0,
			     "Failed to enable alarm");
	}

	/* Wait for alarms to trigger */
	k_msleep(15000);

	/* Validate alarms invoked triggered event handler */
	for (size_t i = 0; i < fixture_instance.rtcc_alarms_count; i++) {
		zassert_true(atomic_test_bit(&fixture_instance.alarms_triggered, i) == true,
			     "Alarm did not trigger");
	}

	/* Validate alarms report as triggered */
	for (size_t i = 0; i < fixture_instance.rtcc_alarms_count; i++) {
		zassert_true(rtcc_alarm_triggered_test(fixture_instance.rtcc, i) == 1,
			     "Alarm not reporting as triggered");
	}

	/*
	 * Reset time to validate alarms will not trigger while triggered
	 */

	/* Set RTCC time */
	zassert_true(rtcc_time_set(fixture_instance.rtcc, &pre_alarm_datetime) == 0,
		     "Failed to set time");

	/* Wait for alarms to trigger */
	k_msleep(15000);

	/* Validate alarms did not invoke triggered event handler */
	for (size_t i = 0; i < fixture_instance.rtcc_alarms_count; i++) {
		zassert_true(atomic_test_bit(&fixture_instance.alarms_triggered, i) == true,
			     "Alarm invoked triggered event handler while triggered");
	}

	/* Validate alarms still report as triggered */
	for (size_t i = 0; i < fixture_instance.rtcc_alarms_count; i++) {
		zassert_true(rtcc_alarm_triggered_test(fixture_instance.rtcc, i) == 1,
			     "Alarm not reporting as triggered");
	}

	/*
	 * Clear alarm triggered state and reset time and wait for alarms to trigger
	 */

	/* Clear alarm triggered state */
	for (size_t i = 0; i < fixture_instance.rtcc_alarms_count; i++) {
		zassert_true(rtcc_alarm_triggered_clear(fixture_instance.rtcc, i) == 0,
			     "Failed to clear alarm triggered state");
	}

	/* Set RTCC time */
	zassert_true(rtcc_time_set(fixture_instance.rtcc, &pre_alarm_datetime) == 0,
		     "Failed to set time");

	/* Wait for alarms to trigger */
	k_msleep(15000);

	/* Validate alarms invoked triggered event handler */
	for (size_t i = 0; i < fixture_instance.rtcc_alarms_count; i++) {
		zassert_true(atomic_test_bit(&fixture_instance.alarms_triggered, i) == true,
			     "Alarm did not trigger");
	}

	/* Validate alarms report as triggered */
	for (size_t i = 0; i < fixture_instance.rtcc_alarms_count; i++) {
		zassert_true(rtcc_alarm_triggered_test(fixture_instance.rtcc, i) == 1,
			     "Alarm not reporting as triggered");
	}
}

ZTEST(rtcc_user, test_timestamp_not_sup)
{
	/* Skip test if timestamps are supported by hardware */
	if (fixture_instance.rtcc_timestamps_count > 0) {
		return;
	}

	zassert_true(rtcc_timestamp_get(fixture_instance.rtcc, 0, NULL) == -ENOTSUP,
		     "Should return -ENOTSUP if not supported by hardware");

	zassert_true(rtcc_timestamp_clear(fixture_instance.rtcc, 0) == -ENOTSUP,
		     "Should return -ENOTSUP if not supported by hardware");
}

/* Resume device using power management before running test suite */
static void *rtcc_user_setup(void)
{
	enum pm_device_state state;

	/* Get device pm state */
	if (pm_device_state_get(fixture_instance.rtcc, &state) == -ENOSYS) {
		/* PM device not supported */
		return NULL;
	}

	/* Enable RTCC using pm device runtime */
	if (pm_device_runtime_is_enabled(fixture_instance.rtcc) == true) {
		zassert_true(pm_device_runtime_get(fixture_instance.rtcc) == 0,
			     "Failed to resume device using pm device runtime");

		return NULL;
	}

	/* Check if already resumed */
	if (state == PM_DEVICE_STATE_ACTIVE) {
		return NULL;
	}

	/* Resume device using pm device action */
	zassert_true(pm_device_action_run(fixture_instance.rtcc, PM_DEVICE_ACTION_RESUME) == 0,
		     "Failed to resume device using pm device action");

	return NULL;
}

/* Set default before every test */
static void rtcc_user_before(void *fixture)
{
	ARG_UNUSED(fixture);

	struct tm datetime_set;
	time_t timer = RTCC_TEST_DEFAULT_TIME;

	gmtime_r(&timer, &datetime_set);

	zassert_true(rtcc_time_set(fixture_instance.rtcc, &datetime_set) == 0,
		     "Failed to set time");

	/* Reset alarms triggered */
	atomic_set(&fixture_instance.alarms_triggered, 0);

	/* Disable all alarms */
	for (size_t i = 0; i < fixture_instance.rtcc_alarms_count; i++) {
		zassert_true(rtcc_alarm_disable(fixture_instance.rtcc, i) == 0,
			     "Failed to enable alarm");
	}

	/* Reset all alarms triggered flags */
	for (size_t i = 0; i < fixture_instance.rtcc_alarms_count; i++) {
		zassert_true(rtcc_alarm_triggered_clear(fixture_instance.rtcc, i) == 0,
			     "Failed to clear alarm triggered state");
	}
}

/* Power down RTCC using power management after running test suite */
static void rtcc_user_teardown(void *fixture)
{
	ARG_UNUSED(fixture);

	enum pm_device_state state;

	/* Get device pm state */
	if (pm_device_state_get(fixture_instance.rtcc, &state) == -ENOSYS) {
		/* PM device not supported */
		return;
	}

	/* Suspend RTCC using pm device runtime */
	if (pm_device_runtime_is_enabled(fixture_instance.rtcc) == true) {
		zassert_true(pm_device_runtime_put(fixture_instance.rtcc) == 0,
			     "Failed to suspend device using pm device runtime");

		return;
	}

	/* Suspend device using pm device action */
	zassert_true(pm_device_action_run(fixture_instance.rtcc, PM_DEVICE_ACTION_SUSPEND) == 0,
		     "Failed to suspend device using pm device action");
}

ZTEST_SUITE(rtcc_user, NULL, rtcc_user_setup, rtcc_user_before, NULL, rtcc_user_teardown);
