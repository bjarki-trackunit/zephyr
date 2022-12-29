/*
 * This driver emululates the behavior of an RTCC.
 *
 * Copyright 2022 Bjarki Arge Andreasen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_rtcc_emul

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/rtcc.h>

#include <time.h>

struct rtcc_emul_data;

struct rtcc_emul_work_delayable {
	struct k_work_delayable dwork;
	const struct device *dev;
};

struct rtcc_emul_alarm {
	struct tm datetime;
	uint32_t datetime_mask;
	rtcc_alarm_triggered_handler triggered_handler;
	void *triggered_handler_user_data;
	bool configured;
	bool enabled;
	bool triggered;
};

struct rtcc_emul_data {
	bool datetime_set;

	struct tm datetime;

	struct k_mutex lock;

	struct rtcc_emul_work_delayable dwork;

	struct rtcc_emul_alarm *alarms;
	size_t alarms_count;
};

static const uint8_t rtcc_emul_days_in_month[12] = {
	31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

static const uint8_t rtcc_emul_days_in_month_with_leap[12] = {
	31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

static bool rtcc_emul_is_leap_year(struct tm *datetime)
{
	if ((datetime->tm_year % 400 == 0) ||
	    (((datetime->tm_year % 100) > 0) && ((datetime->tm_year % 4) == 0))) {
		return true;
	}

	return false;
}

static int rtcc_emul_get_days_in_month(struct tm *datetime)
{
	const uint8_t *dim = (rtcc_emul_is_leap_year(datetime) == true) ?
			     (rtcc_emul_days_in_month_with_leap) :
			     (rtcc_emul_days_in_month);

	return dim[datetime->tm_mon];
}

static void rtcc_emul_increment_tm(struct tm *datetime)
{
	/* Increment second */
	datetime->tm_sec++;

	/* Validate second limit */
	if (datetime->tm_sec < 60) {
		return;
	}

	datetime->tm_sec = 0;

	/* Increment minute */
	datetime->tm_min++;

	/* Validate minute limit */
	if (datetime->tm_min < 60) {
		return;
	}

	datetime->tm_min = 0;

	/* Increment hour */
	datetime->tm_hour++;

	/* Validate hour limit */
	if (datetime->tm_hour < 24) {
		return;
	}

	datetime->tm_hour = 0;

	/* Increment day */
	datetime->tm_wday++;
	datetime->tm_mday++;
	datetime->tm_yday++;

	/* Limit week day */
	if (datetime->tm_wday > 6) {
		datetime->tm_wday = 0;
	}

	/* Validate month limit */
	if (datetime->tm_mday <= rtcc_emul_get_days_in_month(datetime)) {
		return;
	}

	/* Increment month */
	datetime->tm_mon++;

	/* Validate month limit */
	if (datetime->tm_mon < 12) {
		return;
	}

	/* Increment year */
	datetime->tm_mon = 0;
	datetime->tm_yday = 0;
	datetime->tm_year++;
}

static void rtcc_emul_test_alarms(const struct device *dev)
{
	struct rtcc_emul_data *data = (struct rtcc_emul_data *)dev->data;
	struct rtcc_emul_alarm *alarm;

	for (size_t i = 0; i < data->alarms_count; i++) {
		alarm = &data->alarms[i];

		if (alarm->configured == false) {
			continue;
		}

		if (alarm->enabled == false) {
			continue;
		}

		if (alarm->triggered == true) {
			continue;
		}

		if ((alarm->datetime_mask & RTCC_ALARM_MATCH_MASK_SECOND) &&
		    (alarm->datetime.tm_sec != data->datetime.tm_sec)) {
			continue;
		}

		if ((alarm->datetime_mask & RTCC_ALARM_MATCH_MASK_MINUTE) &&
		    (alarm->datetime.tm_min != data->datetime.tm_min)) {
			continue;
		}

		if ((alarm->datetime_mask & RTCC_ALARM_MATCH_MASK_HOUR) &&
		    (alarm->datetime.tm_hour != data->datetime.tm_hour)) {
			continue;
		}

		if ((alarm->datetime_mask & RTCC_ALARM_MATCH_MASK_WEEKDAY) &&
		    (alarm->datetime.tm_wday != data->datetime.tm_wday)) {
			continue;
		}

		if ((alarm->datetime_mask & RTCC_ALARM_MATCH_MASK_MONTH) &&
		    (alarm->datetime.tm_mon != data->datetime.tm_mon)) {
			continue;
		}

		if ((alarm->datetime_mask & RTCC_ALARM_MATCH_MASK_YEAR) &&
		    (alarm->datetime.tm_year != data->datetime.tm_year)) {
			continue;
		}

		alarm->triggered = true;

		if (alarm->triggered_handler == NULL) {
			continue;
		}

		alarm->triggered_handler(dev, (size_t)i, alarm->triggered_handler_user_data);
	}
}

static void rtcc_emul_update(struct k_work *work)
{
	struct rtcc_emul_work_delayable *work_delayable = (struct rtcc_emul_work_delayable *)work;
	const struct device *dev = work_delayable->dev;
	struct rtcc_emul_data *data = (struct rtcc_emul_data *)dev->data;

	k_work_schedule(&work_delayable->dwork, K_MSEC(1000));

	k_mutex_lock(&data->lock, K_FOREVER);

	rtcc_emul_increment_tm(&data->datetime);

	rtcc_emul_test_alarms(dev);

	k_mutex_unlock(&data->lock);
}

static int rtcc_emul_time_set(const struct device *dev, const struct tm *datetime)
{
	struct rtcc_emul_data *data = (struct rtcc_emul_data *)dev->data;

	/* Validate arguments */
	if (datetime == NULL) {
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	data->datetime = (*datetime);

	data->datetime_set = true;

	k_mutex_unlock(&data->lock);

	return 0;
}

static int rtcc_emul_time_get(const struct device *dev, struct tm *datetime)
{
	struct rtcc_emul_data *data = (struct rtcc_emul_data *)dev->data;

	/* Validate arguments */
	if (datetime == NULL) {
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	/* Validate RTCC time is set */
	if (data->datetime_set == false) {
		k_mutex_unlock(&data->lock);
		return -ENODATA;
	}

	(*datetime) = data->datetime;

	k_mutex_unlock(&data->lock);
	return 0;
}

static int rtcc_emul_alarm_config_set(const struct device *dev, size_t id,
				      const struct rtcc_alarm_config *config)
{
	struct rtcc_emul_data *data = (struct rtcc_emul_data *)dev->data;

	/* Validate arguments */
	if (config == NULL) {
		return -EINVAL;
	}

	if ((config->datetime == NULL) ||
	    (config->mask == 0)) {
		return -EINVAL;
	}

	if (data->alarms_count <= id) {
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	data->alarms[id].datetime = (*config->datetime);
	data->alarms[id].datetime_mask = config->mask;
	data->alarms[id].triggered_handler = config->handler;
	data->alarms[id].triggered_handler_user_data = config->user_data;
	data->alarms[id].configured = true;
	data->alarms[id].triggered = false;

	k_mutex_unlock(&data->lock);
	return 0;
}

static int rtcc_emul_alarm_state_get(const struct device *dev, size_t id,
				     struct rtcc_alarm_state *state)
{
	struct rtcc_emul_data *data = (struct rtcc_emul_data *)dev->data;

	if (state == NULL) {
		return -EINVAL;
	}

	if (data->alarms_count <= id) {
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	if (data->alarms[id].configured == false) {
		k_mutex_unlock(&data->lock);
		return -ENODATA;
	}

	state->datetime = data->alarms[id].datetime;
	state->mask = data->alarms[id].datetime_mask;
	state->enabled = data->alarms[id].enabled;
	state->triggered = data->alarms[id].triggered;

	k_mutex_unlock(&data->lock);
	return 0;
}

static int rtcc_emul_alarm_enable(const struct device *dev, size_t id)
{
	struct rtcc_emul_data *data = (struct rtcc_emul_data *)dev->data;

	if (data->alarms_count <= id) {
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	data->alarms[id].enabled = true;

	k_mutex_unlock(&data->lock);
	return 0;
}

static int rtcc_emul_alarm_disable(const struct device *dev, size_t id)
{
	struct rtcc_emul_data *data = (struct rtcc_emul_data *)dev->data;

	if (data->alarms_count <= id) {
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	data->alarms[id].enabled = false;

	k_mutex_unlock(&data->lock);
	return 0;
}

static int rtcc_emul_alarm_triggered_test(const struct device *dev, size_t id)
{
	struct rtcc_emul_data *data = (struct rtcc_emul_data *)dev->data;
	int triggered;

	if (data->alarms_count <= id) {
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	triggered = (data->alarms[id].triggered == true) ? 1 : 0;

	k_mutex_unlock(&data->lock);
	return triggered;
}

static int rtcc_emul_alarm_triggered_clear(const struct device *dev, size_t id)
{
	struct rtcc_emul_data *data = (struct rtcc_emul_data *)dev->data;

	k_mutex_lock(&data->lock, K_FOREVER);

	if (data->alarms_count <= id) {
		k_mutex_unlock(&data->lock);
		return -EINVAL;
	}

	data->alarms[id].triggered = false;

	k_mutex_unlock(&data->lock);
	return 0;
}

static int rtcc_emul_alarm_timestamp_get(const struct device *dev, size_t id,
					 struct tm *timestamp)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(id);
	ARG_UNUSED(timestamp);

	return -ENOTSUP;
}

static int rtcc_emul_alarm_timestamp_clear(const struct device *dev, size_t id)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(id);

	return -ENOTSUP;
}

struct rtcc_driver_api rtcc_emul_driver_api = {
	.time_set = rtcc_emul_time_set,
	.time_get = rtcc_emul_time_get,
	.alarm_config_set = rtcc_emul_alarm_config_set,
	.alarm_state_get = rtcc_emul_alarm_state_get,
	.alarm_enable = rtcc_emul_alarm_enable,
	.alarm_disable = rtcc_emul_alarm_disable,
	.alarm_triggered_test = rtcc_emul_alarm_triggered_test,
	.alarm_triggered_clear = rtcc_emul_alarm_triggered_clear,
	.timestamp_get = rtcc_emul_alarm_timestamp_get,
	.timestamp_clear = rtcc_emul_alarm_timestamp_clear,
};

int rtcc_emul_init(const struct device *dev)
{
	struct rtcc_emul_data *data = (struct rtcc_emul_data *)dev->data;

	k_mutex_init(&data->lock);

	data->dwork.dev = dev;
	k_work_init_delayable(&data->dwork.dwork, rtcc_emul_update);

	k_work_schedule(&data->dwork.dwork, K_MSEC(1000));

	return 0;
}

#define RTCC_EMUL_DEVICE(id)                                                                    \
	static struct rtcc_emul_alarm rtcc_emul_alarms_##id[DT_INST_PROP(id, alarms_count)];    \
												\
	struct rtcc_emul_data rtcc_emul_data_##id = {                                           \
		.alarms = rtcc_emul_alarms_##id,                                                \
		.alarms_count = ARRAY_SIZE(rtcc_emul_alarms_##id),                              \
	};                                                                                      \
												\
	DEVICE_DT_INST_DEFINE(id, rtcc_emul_init, NULL,                                         \
			      &rtcc_emul_data_##id, NULL, POST_KERNEL,                          \
			      99, &rtcc_emul_driver_api);

DT_INST_FOREACH_STATUS_OKAY(RTCC_EMUL_DEVICE);
