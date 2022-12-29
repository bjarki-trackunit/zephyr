/*
 * Copyright (c) 2022 Trackunit Corporation
 * Copyright (c) 2022 Bjarki Arge Andreasen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file drivers/rtc.h
 * @brief Public real time counter driver API
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_RTC_H_
#define ZEPHYR_INCLUDE_DRIVERS_RTC_H_

/**
 * @brief RTC Interface
 * @ingroup io_interfaces
 * @{
 */

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @typedef rtc_alarm_triggered_handler
 * @brief RTC alarm triggered handler
 *
 * @param dev Device instance invoking the handler
 * @param id Alarm id
 * @param user_data Optional user data passed with the alarm configuration
 *
 * @note This handler is likely to be invoked from within an ISR
 * context
 */
typedef void (*rtc_alarm_triggered_handler)(const struct device *dev, size_t id, void *user_data);

/**
 * @brief RTC alarm configuration
 *
 * @param compare Counter compare value at which the handler will be invoked
 * @param handler Handler invoked once the alarm is triggered
 * @param user_data Optional user data passed to the handler when invoked
 *
 * @note This handler is likely to be invoked from within an ISR
 * context
 *
 * @note The handler and user data may be lost when system resets
 */
struct rtc_alarm_config {
	uint32_t compare;
	rtc_alarm_triggered_handler handler;
	void *user_data;
};

/**
 * @brief RTC alarm state
 *
 * @param compare Counter compare value at which the handler will be invoked
 * @param enabled True if alarm is enabled
 * @param triggered True if alarm triggered event has occurred
 */
struct rtc_alarm_state {
	uint32_t compare;
	bool enabled;
	bool triggered;
};

/**
 * @typedef rtc_counter_set
 * @brief API for setting RTC value
 */
typedef int (*rtc_api_counter_set)(const struct device *dev, uint32_t counter);

/**
 * @typedef rtc_counter_get
 * @brief API for getting RTC value
 */
typedef int (*rtc_api_counter_get)(const struct device *dev, uint32_t *counter);

/**
 * @typedef rtc_alarm_config_set
 * @brief API for setting RTC alarm configuration
 */
typedef int (*rtc_api_alarm_config_set)(const struct device *dev, size_t id,
					const struct rtc_alarm_config *config);

/**
 * @typedef rtc_alarm_state_get
 * @brief API for getting RTC alarm state
 */
typedef int (*rtc_api_alarm_state_get)(const struct device *dev, size_t id,
				       struct rtc_alarm_state *state);

/**
 * @typedef rtc_alarm_enable
 * @brief API for enabling RTC alarm
 */
typedef int (*rtc_api_alarm_enable)(const struct device *dev, size_t id);

/**
 * @typedef rtc_alarm_disable
 * @brief API for disabling RTC alarm
 */
typedef int (*rtc_api_alarm_disable)(const struct device *dev, size_t id);

/**
 * @typedef rtc_alarm_triggered_test
 * @brief API for testing if RTC alarm triggered event has occurred
 */
typedef int (*rtc_api_alarm_triggered_test)(const struct device *dev, size_t id);

/**
 * @typedef rtc_alarm_triggered_clear
 * @brief API for clearing RTC alarm triggered event
 */
typedef int (*rtc_api_alarm_triggered_clear)(const struct device *dev, size_t id);

/**
 * @brief RTC driver API
 */
__subsystem struct rtc_driver_api {
	rtc_api_counter_set counter_set;
	rtc_api_counter_get counter_get;
	rtc_api_alarm_config_set alarm_config_set;
	rtc_api_alarm_state_get alarm_state_get;
	rtc_api_alarm_enable alarm_enable;
	rtc_api_alarm_disable alarm_disable;
	rtc_api_alarm_triggered_test alarm_triggered_test;
	rtc_api_alarm_triggered_clear alarm_triggered_clear;
};

/**
 * @brief API for setting RTC value
 *
 * @param dev Device instance
 * @param counter Counter value to set
 *
 * @return 0 if successful
 * @return -ENOSYS if API is not implemented
 * @return -ENOTSUP if API is not supported by hardware
 * @return -errno code if failure
 */
__syscall int rtc_counter_set(const struct device *dev, uint32_t counter);

static inline int z_impl_rtc_counter_set(const struct device *dev, uint32_t counter)
{
	const struct rtc_driver_api *api =
		(const struct rtc_driver_api *)dev->api;

	return api->counter_set(dev, counter);
}

/**
 * @brief API for getting RTC value
 *
 * @param dev Device instance
 * @param counter Destination for the counter value
 *
 * @return 0 if successful
 * @return -ENOSYS if API is not implemented
 * @return -ENOTSUP if API is not supported by hardware
 * @return -errno code if failure
 */
__syscall int rtc_counter_get(const struct device *dev, uint32_t *counter);

static inline int z_impl_rtc_counter_get(const struct device *dev, uint32_t *counter)
{
	const struct rtc_driver_api *api =
		(const struct rtc_driver_api *)dev->api;

	return api->counter_get(dev, counter);
}

/**
 * @brief API for setting RTC alarm configuration
 *
 * @details Setting the configuration must clear the alarm triggered
 * event and leave the alarm enabled state unaltered
 *
 * @param dev Device instance
 * @param id Id of the alarm to configure
 * @param config The alarm configuration to set
 *
 * @return 0 if successful
 * @return -EINVAL if id is out of range or configuration is invalid
 * @return -ENOSYS if API is not implemented
 * @return -ENOTSUP if API is not supported by hardware
 * @return -errno code if failure
 */
__syscall int rtc_alarm_config_set(const struct device *dev, size_t id,
				   const struct rtc_alarm_config *config);

static inline int z_impl_rtc_alarm_config_set(const struct device *dev, size_t id,
					      const struct rtc_alarm_config *config)
{
	const struct rtc_driver_api *api =
		(const struct rtc_driver_api *)dev->api;

	return api->alarm_config_set(dev, id, config);
}

/**
 * @brief API for getting RTC alarm state
 *
 * @param dev Device instance
 * @param id Id of the alarm to get the configuration from
 * @param state Destination for the alarm state
 *
 * @return 0 if successful
 * @return -EINVAL if id is out of range
 * @return -ENOSYS if API is not implemented
 * @return -ENOTSUP if API is not supported by hardware
 * @return -errno code if failure
 */
__syscall int rtc_alarm_state_get(const struct device *dev, size_t id,
				  struct rtc_alarm_state *state);

static inline int z_impl_rtc_alarm_state_get(const struct device *dev, size_t id,
					     struct rtc_alarm_state *state)
{
	const struct rtc_driver_api *api =
		(const struct rtc_driver_api *)dev->api;

	return api->alarm_state_get(dev, id, state);
}

/**
 * @brief API for enabling RTC alarm
 *
 * @param dev Device instance
 * @param id Id of the alarm to enable
 *
 * @note Alarm should be configured before being enabled
 *
 * @return 0 if successful
 * @return -EINVAL if id is out of range
 * @return -ENOSYS if API is not implemented
 * @return -ENOTSUP if API is not supported by hardware
 * @return -errno code if failure
 */
__syscall int rtc_alarm_enable(const struct device *dev, size_t id);

static inline int z_impl_rtc_alarm_enable(const struct device *dev, size_t id)
{
	const struct rtc_driver_api *api =
		(const struct rtc_driver_api *)dev->api;

	return api->alarm_enable(dev, id);
}

/**
 * @brief API for disabling RTC alarm
 *
 * @param dev Device instance
 * @param id Id of the alarm to disable
 *
 * @return 0 if successful
 * @return -EINVAL if id is out of range
 * @return -ENOSYS if API is not implemented
 * @return -ENOTSUP if API is not supported by hardware
 * @return -errno code if failure
 */
__syscall int rtc_alarm_disable(const struct device *dev, size_t id);

static inline int z_impl_rtc_alarm_disable(const struct device *dev, size_t id)
{
	const struct rtc_driver_api *api =
		(const struct rtc_driver_api *)dev->api;

	return api->alarm_disable(dev, id);
}

/**
 * @brief API for testing if RTC alarm triggered event has occurred
 *
 * @param dev Device instance
 * @param id Id of the alarm to test
 *
 * @return 1 if alarm has been triggered
 * @return 0 if alarm has not been triggered
 * @return -EINVAL if id is out of range
 * @return -ENOSYS if API is not implemented
 * @return -ENOTSUP if API is not supported by hardware
 * @return -errno code if failure
 */
__syscall int rtc_alarm_triggered_test(const struct device *dev, size_t id);

static inline int z_impl_rtc_alarm_triggered_test(const struct device *dev, size_t id)
{
	const struct rtc_driver_api *api =
		(const struct rtc_driver_api *)dev->api;

	return api->alarm_triggered_test(dev, id);
}

/**
 * @brief API for clearing RTC alarm triggered event
 *
 * @param dev Device instance
 * @param id Id of the alarm for which the triggered event will be cleared
 *
 * @return 0 if successful
 * @return -EINVAL if id is out of range
 * @return -ENOSYS if API is not implemented
 * @return -ENOTSUP if API is not supported by hardware
 * @return -errno code if failure
 */
__syscall int rtc_alarm_triggered_clear(const struct device *dev, size_t id);

static inline int z_impl_rtc_alarm_triggered_clear(const struct device *dev, size_t id)
{
	const struct rtc_driver_api *api =
		(const struct rtc_driver_api *)dev->api;

	return api->alarm_triggered_clear(dev, id);
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#include <syscalls/rtc.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_RTC_H_ */
