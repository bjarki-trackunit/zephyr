/*
 * Copyright 2022 Bjarki Arge Andreasen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/timeutil.h>
#include <zephyr/ztest.h>

#define RTC_TEST_DEFAULT_COUNT          (10000L)
#define RTC_TEST_SET_GET_COUNT          (11000L)
#define RTC_TEST_SET_COUNT_TOL          (1L)
#define RTC_TEST_ALARM_COMPARE          (RTC_TEST_DEFAULT_COUNT + 10L)

struct rtc_test_fixture {
	/* Device and devicetree config */
	const struct device *rtc;
	const size_t rtc_alarms_count;

	/* Alarm test */
	atomic_t alarms_triggered;
	int32_t alarms_user_data;
};

static struct rtc_test_fixture fixture_instance = {
	.rtc = DEVICE_DT_GET(DT_ALIAS(rtc)),
	.rtc_alarms_count = DT_PROP(DT_ALIAS(rtc), alarms_count),
};

ZTEST(rtc_user, test_counter_set_get)
{
	uint32_t counter_get;

	zassert_true(rtc_counter_set(fixture_instance.rtc, RTC_TEST_SET_GET_COUNT) == 0,
		     "Failed to set counter");

	zassert_true(rtc_counter_get(fixture_instance.rtc, &counter_get) == 0,
		     "Failed to get counter)");

	zassert_true((counter_get >= RTC_TEST_SET_GET_COUNT) &&
		     (counter_get <= (RTC_TEST_SET_GET_COUNT + RTC_TEST_SET_COUNT_TOL)),
		     "Got unexpected counter value");
}

ZTEST(rtc_user, test_alarms_not_sup)
{
	/* Skip test if alarms are supported by hardware */
	if (fixture_instance.rtc_alarms_count > 0) {
		return;
	}

	zassert_true(rtc_alarm_config_set(fixture_instance.rtc, 0, NULL) == -ENOTSUP,
		     "Should return -ENOTSUP if not supported by hardware");

	zassert_true(rtc_alarm_state_get(fixture_instance.rtc, 0, NULL) == -ENOTSUP,
		     "Should return -ENOTSUP if not supported by hardware");

	zassert_true(rtc_alarm_triggered_test(fixture_instance.rtc, 0) == -ENOTSUP,
		     "Should return -ENOTSUP if not supported by hardware");

	zassert_true(rtc_alarm_triggered_clear(fixture_instance.rtc, 0) == -ENOTSUP,
		     "Should return -ENOTSUP if not supported by hardware");
}

static void rtc_user_alarm_handler(const struct device *dev, size_t id, void *user_data)
{
	zassert_true(dev == fixture_instance.rtc,
		     "Unexpected device passed to alarm handler");

	zassert_true(user_data == &fixture_instance.alarms_user_data,
		     "Unexpected user_data passed to alarm handler");

	atomic_set_bit(&fixture_instance.alarms_triggered, id);
}

ZTEST(rtc_user, test_alarms)
{
	struct rtc_alarm_state state;

	zassume_true(fixture_instance.rtc_alarms_count > 0,
		     "RTC does not support alarms");

	struct rtc_alarm_config config = {
		.compare = RTC_TEST_ALARM_COMPARE,
		.handler = rtc_user_alarm_handler,
		.user_data = &fixture_instance.alarms_user_data,
	};

	/*
	 * Configure alarms and verify state
	 */

	/* Configure all alarms */
	for (size_t i = 0; i < fixture_instance.rtc_alarms_count; i++) {
		zassert_true(rtc_alarm_config_set(fixture_instance.rtc, i, &config) == 0,
			     "Failed to configure alarm");
	}

	/* Validate all alarm states */
	for (size_t i = 0; i < fixture_instance.rtc_alarms_count; i++) {
		zassert_true(rtc_alarm_state_get(fixture_instance.rtc, i, &state) == 0,
			     "Failed to get alarm state");

		zassert_true((state.compare == RTC_TEST_ALARM_COMPARE) &&
			     (state.enabled == false) &&
			     (state.triggered == false),
			     "state does not match condiguration");
	}

	/* Validate alarms not reporting as triggered */
	for (size_t i = 0; i < fixture_instance.rtc_alarms_count; i++) {
		zassert_true(rtc_alarm_triggered_test(fixture_instance.rtc, i) == 0,
			     "Alarm not reporting as not triggered");
	}

	/*
	 * Set time to 10 seconds before alarms shall trigger and wait for alarms to
	 * trigger
	 */

	/* Set RTC counter */
	zassert_true(rtc_counter_set(fixture_instance.rtc, RTC_TEST_DEFAULT_COUNT) == 0,
		     "Failed to set counter");

	/* Enable all alarms */
	for (size_t i = 0; i < fixture_instance.rtc_alarms_count; i++) {
		zassert_true(rtc_alarm_enable(fixture_instance.rtc, i) == 0,
			     "Failed to enable alarm");
	}

	/* Wait for alarms to trigger */
	k_msleep(15000);

	/* Validate alarms invoked triggered event handler */
	for (size_t i = 0; i < fixture_instance.rtc_alarms_count; i++) {
		zassert_true(atomic_test_bit(&fixture_instance.alarms_triggered, i) == true,
			     "Alarm did not trigger");
	}

	/* Validate alarms report as triggered */
	for (size_t i = 0; i < fixture_instance.rtc_alarms_count; i++) {
		zassert_true(rtc_alarm_triggered_test(fixture_instance.rtc, i) == 1,
			     "Alarm not reporting as triggered");
	}

	/*
	 * Reset time to validate alarms will not trigger while triggered
	 */

	/* Set RTC counter */
	zassert_true(rtc_counter_set(fixture_instance.rtc, RTC_TEST_DEFAULT_COUNT) == 0,
		     "Failed to set time");

	/* Wait for alarms to trigger */
	k_msleep(15000);

	/* Validate alarms did not invoke triggered event handler */
	for (size_t i = 0; i < fixture_instance.rtc_alarms_count; i++) {
		zassert_true(atomic_test_bit(&fixture_instance.alarms_triggered, i) == true,
			     "Alarm invoked triggered event handler while triggered");
	}

	/* Validate alarms still report as triggered */
	for (size_t i = 0; i < fixture_instance.rtc_alarms_count; i++) {
		zassert_true(rtc_alarm_triggered_test(fixture_instance.rtc, i) == 1,
			     "Alarm not reporting as triggered");
	}

	/*
	 * Clear alarm triggered state and reset time and wait for alarms to trigger
	 */

	/* Clear alarm triggered state */
	for (size_t i = 0; i < fixture_instance.rtc_alarms_count; i++) {
		zassert_true(rtc_alarm_triggered_clear(fixture_instance.rtc, i) == 0,
			     "Failed to clear alarm triggered state");
	}

	/* Set RTC counter */
	zassert_true(rtc_counter_set(fixture_instance.rtc, RTC_TEST_DEFAULT_COUNT) == 0,
		     "Failed to set time");

	/* Wait for alarms to trigger */
	k_msleep(15000);

	/* Validate alarms invoked triggered event handler */
	for (size_t i = 0; i < fixture_instance.rtc_alarms_count; i++) {
		zassert_true(atomic_test_bit(&fixture_instance.alarms_triggered, i) == true,
			     "Alarm did not trigger");
	}

	/* Validate alarms report as triggered */
	for (size_t i = 0; i < fixture_instance.rtc_alarms_count; i++) {
		zassert_true(rtc_alarm_triggered_test(fixture_instance.rtc, i) == 1,
			     "Alarm not reporting as triggered");
	}
}

/* Resume device using power management before running test suite */
static void *rtc_user_setup(void)
{
	enum pm_device_state state;

	/* Get device pm state */
	if (pm_device_state_get(fixture_instance.rtc, &state) == -ENOSYS) {
		/* PM device not supported */
		return NULL;
	}

	/* Enable RTC using pm device runtime */
	if (pm_device_runtime_is_enabled(fixture_instance.rtc) == true) {
		zassert_true(pm_device_runtime_get(fixture_instance.rtc) == 0,
			     "Failed to resume device using pm device runtime");

		return NULL;
	}

	/* Check if already resumed */
	if (state == PM_DEVICE_STATE_ACTIVE) {
		return NULL;
	}

	/* Resume device using pm device action */
	zassert_true(pm_device_action_run(fixture_instance.rtc, PM_DEVICE_ACTION_RESUME) == 0,
		     "Failed to resume device using pm device action");

	return NULL;
}

/* Set default before every test */
static void rtc_user_before(void *fixture)
{
	ARG_UNUSED(fixture);

	/* Set default time */
	zassert_true(rtc_counter_set(fixture_instance.rtc, RTC_TEST_DEFAULT_COUNT) == 0,
		     "Failed to set time");

	/* Reset alarms triggered */
	atomic_set(&fixture_instance.alarms_triggered, 0);

	/* Disable all alarms */
	for (size_t i = 0; i < fixture_instance.rtc_alarms_count; i++) {
		zassert_true(rtc_alarm_disable(fixture_instance.rtc, i) == 0,
			     "Failed to enable alarm");
	}

	/* Reset all alarms triggered flags */
	for (size_t i = 0; i < fixture_instance.rtc_alarms_count; i++) {
		zassert_true(rtc_alarm_triggered_clear(fixture_instance.rtc, i) == 0,
			     "Failed to clear alarm triggered state");
	}
}

/* Power down RTC using power management after running test suite */
static void rtc_user_teardown(void *fixture)
{
	ARG_UNUSED(fixture);

	enum pm_device_state state;

	/* Get device pm state */
	if (pm_device_state_get(fixture_instance.rtc, &state) == -ENOSYS) {
		/* PM device not supported */
		return;
	}

	/* Suspend RTC using pm device runtime */
	if (pm_device_runtime_is_enabled(fixture_instance.rtc) == true) {
		zassert_true(pm_device_runtime_put(fixture_instance.rtc) == 0,
			     "Failed to suspend device using pm device runtime");

		return;
	}

	/* Suspend device using pm device action */
	zassert_true(pm_device_action_run(fixture_instance.rtc, PM_DEVICE_ACTION_SUSPEND) == 0,
		     "Failed to suspend device using pm device action");
}

ZTEST_SUITE(rtc_user, NULL, rtc_user_setup, rtc_user_before, NULL, rtc_user_teardown);
