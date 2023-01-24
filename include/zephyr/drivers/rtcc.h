/*
 * Copyright (c) 2022 Trackunit Corporation
 * Copyright (c) 2022 Bjarki Arge Andreasen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file drivers/rtcc.h
 * @brief Public real time clock calendar driver API
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_RTCC_H_
#define ZEPHYR_INCLUDE_DRIVERS_RTCC_H_

/**
 * @brief RTCC Interface
 * @ingroup io_interfaces
 * @{
 */

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <errno.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Mask for tm fields to trigger alarm
 * @{
 */
#define RTCC_ALARM_MATCH_MASK_SECOND    BIT(0)
#define RTCC_ALARM_MATCH_MASK_MINUTE    BIT(1)
#define RTCC_ALARM_MATCH_MASK_HOUR      BIT(2)
#define RTCC_ALARM_MATCH_MASK_WEEKDAY   BIT(3)
#define RTCC_ALARM_MATCH_MASK_MONTHDAY  BIT(4)
#define RTCC_ALARM_MATCH_MASK_MONTH     BIT(5)
#define RTCC_ALARM_MATCH_MASK_YEAR      BIT(6)
/**
 * @}
 */

/**
 * @typedef rtcc_alarm_triggered_handler
 * @brief RTCC alarm triggered handler
 *
 * @param dev Device instance invoking the handler
 * @param id Alarm id
 * @param user_data Optional user data passed with the alarm configuration
 *
 * @note This handler is likely to be invoked from within an ISR
 * context
 */
typedef void (*rtcc_alarm_triggered_handler)(const struct device *dev, size_t id, void *user_data);

/**
 * @brief RTCC alarm configuration
 *
 * @param datetime Date and time to match
 * @param mask Parameters of the date and time to match
 * @param handler Handler invoked once the alarm is triggered
 * @param user_data Optional user data passed to the handler when invoked
 *
 * @note The handler is likely to be invoked from within an ISR
 * context
 *
 * @note The handler and user data may be lost when system resets
 */
struct rtcc_alarm_config {
	struct tm *datetime;
	uint32_t mask;
	rtcc_alarm_triggered_handler handler;
	void *user_data;
};

/**
 * @brief RTCC alarm state
 *
 * @param datetime Date and time to match
 * @param mask Parameters of the date and time to match
 * @param enabled True if alarm is enabled
 * @param triggered True if alarm triggered event has occurred
 */
struct rtcc_alarm_state {
	struct tm datetime;
	uint32_t mask;
	bool enabled;
	bool triggered;
};

/**
 * @typedef rtcc_time_set
 * @brief API for setting RTCC date and time
 */
typedef int (*rtcc_api_time_set)(const struct device *dev, const struct tm *datetime);

/**
 * @typedef rtcc_time_get
 * @brief API for getting RTCC date and time
 */
typedef int (*rtcc_api_time_get)(const struct device *dev, struct tm *datetime);

/**
 * @typedef rtcc_alarm_config_set
 * @brief API for setting RTCC alarm configuration
 */
typedef int (*rtcc_api_alarm_config_set)(const struct device *dev, size_t id,
					 const struct rtcc_alarm_config *config);

/**
 * @typedef rtcc_alarm_state_get
 * @brief API for getting RTCC alarm state
 */
typedef int (*rtcc_api_alarm_state_get)(const struct device *dev, size_t id,
					struct rtcc_alarm_state *state);

/**
 * @typedef rtcc_alarm_enable
 * @brief API for enabling RTCC alarm
 */
typedef int (*rtcc_api_alarm_enable)(const struct device *dev, size_t id);

/**
 * @typedef rtcc_alarm_disable
 * @brief API for disabling RTCC alarm
 */
typedef int (*rtcc_api_alarm_disable)(const struct device *dev, size_t id);

/**
 * @typedef rtcc_alarm_triggered_test
 * @brief API for testing if RTCC alarm triggered event has occurred
 */
typedef int (*rtcc_api_alarm_triggered_test)(const struct device *dev, size_t id);

/**
 * @typedef rtcc_alarm_triggered_clear
 * @brief API for clearing RTCC alarm triggered event
 */
typedef int (*rtcc_api_alarm_triggered_clear)(const struct device *dev, size_t id);

/**
 * @typedef rtcc_timestamp_get
 * @brief API for getting RTCC timestamp
 */
typedef int (*rtcc_api_timestamp_get)(const struct device *dev, size_t id, struct tm *timestamp);

/**
 * @typedef rtcc_timestamp_clear
 * @brief API for clearing RTCC timestamp
 */
typedef int (*rtcc_api_timestamp_clear)(const struct device *dev, size_t id);

/**
 * @typedef rtcc_calibration_set
 * @brief API for setting RTCC calibration
 */
typedef int (*rtcc_api_calibration_set)(const struct device *dev, int32_t correction);

/**
 * @typedef rtcc_calibration_get
 * @brief API for getting RTCC calibration
 */
typedef int (*rtcc_api_calibration_get)(const struct device *dev, int32_t *correction);

/**
 * @brief RTCC driver API
 */
__subsystem struct rtcc_driver_api {
	rtcc_api_time_set time_set;
	rtcc_api_time_get time_get;
	rtcc_api_alarm_config_set alarm_config_set;
	rtcc_api_alarm_state_get alarm_state_get;
	rtcc_api_alarm_enable alarm_enable;
	rtcc_api_alarm_disable alarm_disable;
	rtcc_api_alarm_triggered_test alarm_triggered_test;
	rtcc_api_alarm_triggered_clear alarm_triggered_clear;
	rtcc_api_timestamp_get timestamp_get;
	rtcc_api_timestamp_clear timestamp_clear;
        rtcc_api_calibration_set calibration_set;
        rtcc_api_calibration_get calibration_get;
};

/**
 * @brief API for setting RTCC date and time
 *
 * @param dev Device instance
 * @param datetime The date and time to set
 *
 * @return 0 if successful
 * @return -EINVAL if datetime is invalid
 * @return -ENOSYS if API is not implemented
 * @return -ENOTSUP if API is not supported by hardware
 * @return -errno code if failure
 */
__syscall int rtcc_time_set(const struct device *dev, const struct tm *datetime);

static inline int z_impl_rtcc_time_set(const struct device *dev, const struct tm *datetime)
{
	const struct rtcc_driver_api *api =
		(const struct rtcc_driver_api *)dev->api;

	return api->time_set(dev, datetime);
}

/**
 * @brief API for getting RTCC date and time
 *
 * @param dev Device instance
 * @param datetime Destination for the date and time
 *
 * @return 0 if successful
 * @return -ENODATA if time has not been set
 * @return -ENOSYS if API is not implemented
 * @return -ENOTSUP if API is not supported by hardware
 * @return -errno code if failure
 */
__syscall int rtcc_time_get(const struct device *dev, struct tm *datetime);

static inline int z_impl_rtcc_time_get(const struct device *dev, struct tm *datetime)
{
	const struct rtcc_driver_api *api =
		(const struct rtcc_driver_api *)dev->api;

	return api->time_get(dev, datetime);
}

/**
 * @brief API for setting RTCC alarm configuration
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
__syscall int rtcc_alarm_config_set(const struct device *dev, size_t id,
				    const struct rtcc_alarm_config *config);

static inline int z_impl_rtcc_alarm_config_set(const struct device *dev, size_t id,
					       const struct rtcc_alarm_config *config)
{
	const struct rtcc_driver_api *api =
		(const struct rtcc_driver_api *)dev->api;

	return api->alarm_config_set(dev, id, config);
}

/**
 * @brief API for getting RTCC alarm state
 *
 * @param dev Device instance
 * @param id Id of the alarm to get the configuration from
 * @param state Destination for the alarm state
 *
 * @return 0 if successful
 * @return -ENODATA if alarm has not been configured
 * @return -EINVAL if id is out of range
 * @return -ENOSYS if API is not implemented
 * @return -ENOTSUP if API is not supported by hardware
 * @return -errno code if failure
 */
__syscall int rtcc_alarm_state_get(const struct device *dev, size_t id,
				   struct rtcc_alarm_state *state);

static inline int z_impl_rtcc_alarm_state_get(const struct device *dev, size_t id,
					      struct rtcc_alarm_state *state)
{
	const struct rtcc_driver_api *api =
		(const struct rtcc_driver_api *)dev->api;

	return api->alarm_state_get(dev, id, state);
}

/**
 * @brief API for enabling RTCC alarm
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
__syscall int rtcc_alarm_enable(const struct device *dev, size_t id);

static inline int z_impl_rtcc_alarm_enable(const struct device *dev, size_t id)
{
	const struct rtcc_driver_api *api =
		(const struct rtcc_driver_api *)dev->api;

	return api->alarm_enable(dev, id);
}

/**
 * @brief API for disabling RTCC alarm
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
__syscall int rtcc_alarm_disable(const struct device *dev, size_t id);

static inline int z_impl_rtcc_alarm_disable(const struct device *dev, size_t id)
{
	const struct rtcc_driver_api *api =
		(const struct rtcc_driver_api *)dev->api;

	return api->alarm_disable(dev, id);
}

/**
 * @brief API for testing if RTCC alarm triggered event has occurred
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
__syscall int rtcc_alarm_triggered_test(const struct device *dev, size_t id);

static inline int z_impl_rtcc_alarm_triggered_test(const struct device *dev, size_t id)
{
	const struct rtcc_driver_api *api =
		(const struct rtcc_driver_api *)dev->api;

	return api->alarm_triggered_test(dev, id);
}

/**
 * @brief API for clearing RTCC alarm triggered event
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
__syscall int rtcc_alarm_triggered_clear(const struct device *dev, size_t id);

static inline int z_impl_rtcc_alarm_triggered_clear(const struct device *dev, size_t id)
{
	const struct rtcc_driver_api *api =
		(const struct rtcc_driver_api *)dev->api;

	return api->alarm_triggered_clear(dev, id);
}

/**
 * @brief API for getting RTCC timestamp
 *
 * @param dev Device instance
 * @param id Id of the timestamp to get
 * @param timestamp Destination for the current date and time
 *
 * @return 0 if successful
 * @return -ENODATA if timestamp is not set
 * @return -EINVAL if id is out of range
 * @return -ENOSYS if API is not implemented
 * @return -ENOTSUP if API is not supported by hardware
 * @return -errno code if failure
 */
__syscall int rtcc_timestamp_get(const struct device *dev, size_t id, struct tm *timestamp);

static inline int z_impl_rtcc_timestamp_get(const struct device *dev, size_t id,
					    struct tm *timestamp)
{
	const struct rtcc_driver_api *api =
		(const struct rtcc_driver_api *)dev->api;

	return api->timestamp_get(dev, id, timestamp);
}

/**
 * @brief API for clearing RTCC timestamp
 *
 * @param dev Device instance
 * @param id Id of the timestamp to clear
 *
 * @return 0 if successful
 * @return -EINVAL if id is out of range
 * @return -ENOSYS if API is not implemented
 * @return -ENOTSUP if API is not supported by hardware
 * @return -errno code if failure
 */
__syscall int rtcc_timestamp_clear(const struct device *dev, size_t id);

static inline int z_impl_rtcc_timestamp_clear(const struct device *dev, size_t id)
{
	const struct rtcc_driver_api *api =
		(const struct rtcc_driver_api *)dev->api;

	return api->timestamp_clear(dev, id);
}

__syscall int rtcc_calibration_set(const struct device *dev, int32_t correction);

static inline int z_impl_rtcc_calibration_set(const struct device *dev, int32_t correction)
{
	const struct rtcc_driver_api *api =
		(const struct rtcc_driver_api *)dev->api;

	return api->calibration_set(dev, correction);
}

__syscall int rtcc_calibration_get(const struct device *dev, int32_t *correction);

static inline int z_impl_rtcc_calibration_get(const struct device *dev, int32_t *correction)
{
	const struct rtcc_driver_api *api =
		(const struct rtcc_driver_api *)dev->api;

	return api->calibration_get(dev, correction);
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#include <syscalls/rtcc.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_RTCC_H_ */
