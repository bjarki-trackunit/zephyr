/*
 * Copyright (c) 2022 Trackunit Corporation
 * Copyright (c) 2022 Bjarki Arge Andreasen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file drivers/rtc.h
 * @brief Public real time clock driver API
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_RTC_H_
#define ZEPHYR_INCLUDE_DRIVERS_RTC_H_

/**
 * @brief RTC Interface
 * @defgroup rtc_interface RTC Interface
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
 * @brief Mask for alarm time fields to enable when setting alarm time
 * @{
 */
#define RTC_ALARM_TIME_MASK_SECOND      BIT(0)
#define RTC_ALARM_TIME_MASK_MINUTE      BIT(1)
#define RTC_ALARM_TIME_MASK_HOUR        BIT(2)
#define RTC_ALARM_TIME_MASK_MONTHDAY    BIT(3)
#define RTC_ALARM_TIME_MASK_MONTH       BIT(4)
#define RTC_ALARM_TIME_MASK_YEAR        BIT(5)
#define RTC_ALARM_TIME_MASK_WEEKDAY     BIT(6)
#define RTC_ALARM_TIME_MASK_YEARDAY     BIT(7)
/**
 * @}
 */

/**
 * @brief Structure for storing date and time values with sub-second precision.
 */
struct rtc_time {
	int tm_sec;	/**< Seconds [0, 59] */
	int tm_min;	/**< Minutes [0, 59] */
	int tm_hour;	/**< Hours [0, 23] */
	int tm_mday;	/**< Day of the month [1, 31] */
	int tm_mon;	/**< Month [0, 11] */
	int tm_year;	/**< Year - 1900 */
	int tm_wday;	/**< Day of the week [0, 6] (Sunday = 0) (Unknown = -1) */
	int tm_yday;	/**< Day of the year [0, 365] (Unknown = -1) */
	int tm_isdst;	/**< Daylight saving time flag [-1] (Unknown = -1) */
	int tm_nsec;	/**< Nanoseconds [0, 999999999] (Unknown = 0) */
};

/**
 * @typedef rtc_update_irq_callback
 * @brief RTC update event IRQ callback
 *
 * @param dev Device instance invoking the handler
 * @param user_data Optional user data provided when update irq callback is set
 */
typedef void (*rtc_update_irq_callback)(const struct device *dev, void *user_data);

/**
 * @typedef rtc_alarm_irq_callback
 * @brief RTC alarm triggered IRQ callback
 *
 * @param dev Device instance invoking the handler
 * @param id Alarm id
 * @param user_data Optional user data passed with the alarm configuration
 */
typedef void (*rtc_alarm_irq_callback)(const struct device *dev, uint16_t id, void *user_data);

/**
 * @typedef rtc_api_set_time
 * @brief API for setting RTC time
 */
typedef int (*rtc_api_set_time)(const struct device *dev, const struct rtc_time *timeptr);

/**
 * @typedef rtc_api_get_time
 * @brief API for getting RTC time
 */
typedef int (*rtc_api_get_time)(const struct device *dev, struct rtc_time *timeptr);

/**
 * @typedef rtc_api_alarm_get_supported_fields
 * @brief API for getting the supported fields of the RTC alarm time
 */
typedef int (*rtc_api_alarm_get_supported_fields)(const struct device *dev, uint16_t *mask);

/**
 * @typedef rtc_api_alarm_set_time
 * @brief API for setting RTC alarm time
 */
typedef int (*rtc_api_alarm_set_time)(const struct device *dev, uint16_t id, uint16_t mask,
				      const struct rtc_time *timeptr);

/**
 * @typedef rtc_api_alarm_get_time
 * @brief API for getting RTC alarm time
 */
typedef int (*rtc_api_alarm_get_time)(const struct device *dev, uint16_t id, uint16_t *mask,
				      struct rtc_time *timeptr);

/**
 * @typedef rtc_api_alarm_is_pending
 * @brief API for testing if any RTC alarm is pending
 */
typedef int (*rtc_api_alarm_is_pending)(const struct device *dev, uint16_t id);

/**
 * @typedef rtc_api_update_set_irq_callback
 * @brief API for setting alarm IRQ callback
 */
typedef int (*rtc_api_alarm_set_irq_callback)(const struct device *dev,
					      rtc_alarm_irq_callback callback, void *user_data);

/**
 * @typedef rtc_api_alarm_set_irq_callback
 * @brief API for setting alarm IRQ callback
 */
typedef int (*rtc_api_alarm_set_irq_callback)(const struct device *dev,
					      rtc_alarm_irq_callback callback, void *user_data);

/**
 * @typedef rtc_api_alarm_enable_irq
 * @brief API for enabling RTC alarm IRQ
 */
typedef int (*rtc_api_alarm_enable_irq)(const struct device *dev, uint16_t id);

/**
 * @typedef rtc_api_alarm_disable_irq
 * @brief API for disabling RTC alarm IRQ
 */
typedef int (*rtc_api_alarm_disable_irq)(const struct device *dev, uint16_t id);

/**
 * @typedef rtc_api_update_set_irq_callback
 * @brief API for setting update IRQ callback
 */
typedef int (*rtc_api_update_set_irq_callback)(const struct device *dev,
					       rtc_update_irq_callback callback, void *user_data);

/**
 * @typedef rtc_api_update_enable_irq
 * @brief API for enabling RTC update IRQ
 */
typedef int (*rtc_api_update_enable_irq)(const struct device *dev);

/**
 * @typedef rtc_api_update_disable_irq
 * @brief API for disabling RTC update IRQ
 */
typedef int (*rtc_api_update_disable_irq)(const struct device *dev);

/**
 * @typedef rtc_api_set_calibration
 * @brief API for setting RTC calibration
 */
typedef int (*rtc_api_set_calibration)(const struct device *dev, int32_t calibration);

/**
 * @typedef rtc_api_get_calibration
 * @brief API for getting RTC calibration
 */
typedef int (*rtc_api_get_calibration)(const struct device *dev, int32_t *calibration);

/**
 * @brief RTC driver API
 */
__subsystem struct rtc_driver_api {
	rtc_api_set_time set_time;
	rtc_api_get_time get_time;
	rtc_api_alarm_get_supported_fields alarm_get_supported_fields;
	rtc_api_alarm_set_time alarm_set_time;
	rtc_api_alarm_get_time alarm_get_time;
	rtc_api_alarm_is_pending alarm_is_pending;
	rtc_api_alarm_set_irq_callback alarm_set_irq_callback;
	rtc_api_alarm_enable_irq alarm_enable_irq;
	rtc_api_alarm_disable_irq alarm_disable_irq;
	rtc_api_update_set_irq_callback update_set_irq_callback;
	rtc_api_update_enable_irq update_enable_irq;
	rtc_api_update_disable_irq update_disable_irq;
	rtc_api_set_calibration set_calibration;
	rtc_api_get_calibration get_calibration;
};

/**
 * @brief API for setting RTC time
 *
 * @param dev Device instance
 * @param timeptr The time to set
 *
 * @return 0 if successful
 * @return -EINVAL if time is invalid or exceeds hardware capabilities
 * @return -errno code if failure
 */
__syscall int rtc_set_time(const struct device *dev, const struct rtc_time *timeptr);

static inline int z_impl_rtc_set_time(const struct device *dev, const struct rtc_time *timeptr)
{
	const struct rtc_driver_api *api =
		(const struct rtc_driver_api *)dev->api;

	return api->set_time(dev, timeptr);
}

/**
 * @brief API for getting RTC time
 *
 * @param dev Device instance
 * @param timeptr Destination for the time
 *
 * @return 0 if successful
 * @return -ENODATA if time has not been set
 * @return -errno code if failure
 */
__syscall int rtc_get_time(const struct device *dev, struct rtc_time *timeptr);

static inline int z_impl_rtc_get_time(const struct device *dev, struct rtc_time *timeptr)
{
	const struct rtc_driver_api *api =
		(const struct rtc_driver_api *)dev->api;

	return api->get_time(dev, timeptr);
}

/**
 * @brief API for getting the supported fields of the RTC alarm time
 *
 * @param dev Device instance
 * @param mask Mask of fields in the alarm time which are supported
 *
 * @return 0 if successful
 * @return -ENOTSUP if API is not supported by hardware
 * @return -errno code if failure
 */
__syscall int rtc_alarm_get_supported_fields(const struct device *dev, uint16_t *mask);

static inline int z_impl_rtc_alarm_get_supported_fields(const struct device *dev, uint16_t *mask)
{
	const struct rtc_driver_api *api =
		(const struct rtc_driver_api *)dev->api;

	return api->alarm_get_supported_fields(dev, mask);
}

/**
 * @brief API for setting RTC alarm time
 *
 * @details To enable an RTC alarm, one or more fields of the RTC alarm time
 * must be enabled. The mask designates which fields of the RTC alarm time to
 * enable. If the mask parameter is 0, the alarm will be disabled. The RTC
 * alarm will trigger when all enabled fields of the alarm time match the RTC
 * time.
 *
 * @param dev Device instance
 * @param id Id of the alarm
 * @param mask Mask of fields in the alarm time to enable
 * @param timeptr The alarm time to set
 *
 * @note The timeptr param may be NULL if the mask param is 0
 *
 * @note Only the enabled fields in the timeptr param need to be set
 *
 * @note Bits in the mask param are defined in the RTC_ALARM_TIME_MASK defines
 *
 * @return 0 if successful
 * @return -EINVAL if id is out of range or time is invalid
 * @return -ENOTSUP if API is not supported by hardware
 * @return -errno code if failure
 */
__syscall int rtc_alarm_set_time(const struct device *dev, uint16_t id, uint16_t mask,
				 const struct rtc_time *timeptr);

static inline int z_impl_rtc_alarm_set_time(const struct device *dev, uint16_t id, uint16_t mask,
					    const struct rtc_time *timeptr)
{
	const struct rtc_driver_api *api =
		(const struct rtc_driver_api *)dev->api;

	return api->alarm_set_time(dev, id, mask, timeptr);
}

/**
 * @brief API for getting RTC alarm time
 *
 * @param dev Device instance
 * @param id Id of the alarm
 * @param mask Destination for mask of fields which are enabled in the alarm time
 * @param timeptr Destination for the alarm time
 *
 * @note bits in mask are stored in RTC_ALARM_TIME_MASK defines
 *
 * @return 0 if successful
 * @return -EINVAL if id is out of range
 * @return -ENOTSUP if API is not supported by hardware
 * @return -errno code if failure
 */
__syscall int rtc_alarm_get_time(const struct device *dev, uint16_t id, uint16_t *mask,
				 struct rtc_time *timeptr);

static inline int z_impl_rtc_alarm_get_time(const struct device *dev, uint16_t id, uint16_t *mask,
					    struct rtc_time *timeptr)
{
	const struct rtc_driver_api *api =
		(const struct rtc_driver_api *)dev->api;

	return api->alarm_get_time(dev, id, mask, timeptr);
}

/**
 * @brief API for testing if RTC alarm is pending
 *
 * @details Test whether or not the alarm with id is pending. If the alarm
 * is pending, the pending status is cleared.
 *
 * @param dev Device instance
 * @param id Id of the alarm to test
 *
 * @return 1 if alarm was pending
 * @return 0 if alarm was not pending
 * @return -EINVAL if id is out of range
 * @return -ENOTSUP if API is not supported by hardware
 * @return -errno code if failure
 */
__syscall int rtc_alarm_is_pending(const struct device *dev, uint16_t id);

static inline int z_impl_rtc_alarm_is_pending(const struct device *dev, uint16_t id)
{
	const struct rtc_driver_api *api =
		(const struct rtc_driver_api *)dev->api;

	return api->alarm_is_pending(dev, id);
}

/**
 * @brief API for setting alarm IRQ callback
 *
 * @param dev Device instance
 * @param callback Callback called when alarm IRQ occurs
 * @param user_data Optional user data passed to callback
 *
 * @return 0 if successful
 * @return -EINVAL if id is out of range
 * @return -ENOTSUP if API is not supported by hardware
 * @return -errno code if failure
 */
__syscall int rtc_alarm_set_irq_callback(const struct device *dev, rtc_alarm_irq_callback callback,
					 void *user_data);

static inline int z_impl_rtc_alarm_set_irq_callback(const struct device *dev,
						    rtc_alarm_irq_callback callback,
						    void *user_data)
{
	const struct rtc_driver_api *api =
		(const struct rtc_driver_api *)dev->api;

	return api->alarm_set_irq_callback(dev, callback, user_data);
}

/**
 * @brief API for enabling RTC alarm interrupt
 *
 * @param dev Device instance
 * @param id Id of the alarm to enable interrupt for
 *
 * @note Alarm should be configured before being enabled
 *
 * @return 0 if successful
 * @return -EINVAL if id is out of range
 * @return -ENOTSUP if API is not supported by hardware
 * @return -errno code if failure
 */
__syscall int rtc_alarm_enable_irq(const struct device *dev, uint16_t id);

static inline int z_impl_rtc_alarm_enable_irq(const struct device *dev, uint16_t id)
{
	const struct rtc_driver_api *api =
		(const struct rtc_driver_api *)dev->api;

	return api->alarm_enable_irq(dev, id);
}

/**
 * @brief API for disabling RTC alarm interrupt
 *
 * @param dev Device instance
 * @param id Id of the alarm to disable interrupt for
 *
 * @return 0 if successful
 * @return -EINVAL if id is out of range
 * @return -ENOTSUP if API is not supported by hardware
 * @return -errno code if failure
 */
__syscall int rtc_alarm_disable_irq(const struct device *dev, uint16_t id);

static inline int z_impl_rtc_alarm_disable_irq(const struct device *dev, uint16_t id)
{
	const struct rtc_driver_api *api =
		(const struct rtc_driver_api *)dev->api;

	return api->alarm_disable_irq(dev, id);
}

/**
 * @brief API for setting update IRQ callback
 *
 * @details The update IRQ occurs every time the RTC clock is updated by
 * 1 second.
 *
 * @param dev Device instance
 * @param callback Callback called when update IRQ occurs
 * @param user_data Optional user data passed to callback
 *
 * @return 0 if successful
 * @return -ENOTSUP if API is not supported by hardware
 * @return -errno code if failure
 */
__syscall int rtc_update_set_irq_callback(const struct device *dev,
					      rtc_update_irq_callback callback,
					      void *user_data);

static inline int z_impl_rtc_update_set_irq_callback(const struct device *dev,
						     rtc_update_irq_callback callback,
						     void *user_data)
{
	const struct rtc_driver_api *api =
		(const struct rtc_driver_api *)dev->api;

	return api->update_set_irq_callback(dev, callback, user_data);
}

/**
 * @brief API for enabling RTC update IRQ
 *
 * @param dev Device instance
 *
 * @return 0 if successful
 * @return -ENOTSUP if API is not supported by hardware
 * @return -errno code if failure
 */
__syscall int rtc_update_enable_irq(const struct device *dev);

static inline int z_impl_rtc_update_enable_irq(const struct device *dev)
{
	const struct rtc_driver_api *api =
		(const struct rtc_driver_api *)dev->api;

	return api->update_enable_irq(dev);
}

/**
 * @brief API for disabling RTC update IRQ
 *
 * @param dev Device instance
 *
 * @return 0 if successful
 * @return -ENOTSUP if API is not supported by hardware
 * @return -errno code if failure
 */
__syscall int rtc_update_disable_irq(const struct device *dev);

static inline int z_impl_rtc_update_disable_irq(const struct device *dev)
{
	const struct rtc_driver_api *api =
		(const struct rtc_driver_api *)dev->api;

	return api->update_disable_irq(dev);
}

/**
 * @brief API for setting RTC calibration
 *
 * @details Calibration is applied to the RTC clock input
 *
 * @param dev Device instance
 * @param calibration Calibration to set in parts per billion
 *
 * @return 0 if successful
 * @return -EINVAL if calibration is out of range
 * @return -ENOTSUP if API is not supported by hardware
 * @return -errno code if failure
 */
__syscall int rtc_set_calibration(const struct device *dev, int32_t calibration);

static inline int z_impl_rtc_set_calibration(const struct device *dev, int32_t calibration)
{
	const struct rtc_driver_api *api =
		(const struct rtc_driver_api *)dev->api;

	return api->set_calibration(dev, calibration);
}

/**
 * @brief API for getting RTC calibration
 *
 * @param dev Device instance
 * @param calibration Destination for calibration in parts per billion
 *
 * @return 0 if successful
 * @return -ENOTSUP if API is not supported by hardware
 * @return -errno code if failure
 */
__syscall int rtc_get_calibration(const struct device *dev, int32_t *calibration);

static inline int z_impl_rtc_get_calibration(const struct device *dev, int32_t *calibration)
{
	const struct rtc_driver_api *api =
		(const struct rtc_driver_api *)dev->api;

	return api->get_calibration(dev, calibration);
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#include <syscalls/rtc.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_RTC_H_ */
