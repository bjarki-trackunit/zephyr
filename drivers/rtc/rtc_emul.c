/*
 * This driver emululates the behavior of an RTC.
 *
 * Copyright 2022 Bjarki Arge Andreasen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_rtc_emul

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/rtc.h>

struct rtc_emul_data;

struct rtc_emul_work_delayable {
	struct k_work_delayable dwork;
	const struct device *dev;
};

struct rtc_emul_alarm {
	uint32_t compare;
	rtc_alarm_triggered_handler triggered_handler;
	void *triggered_handler_user_data;
	bool enabled;
	bool triggered;
};

struct rtc_emul_data {
	uint32_t counter;

	struct k_mutex lock;

	struct rtc_emul_work_delayable dwork;

	struct rtc_emul_alarm *alarms;
	size_t alarms_count;
};

static void rtc_emul_test_alarms(const struct device *dev)
{
	struct rtc_emul_data *data = (struct rtc_emul_data *)dev->data;
	struct rtc_emul_alarm *alarm;

	for (size_t i = 0; i < data->alarms_count; i++) {
		alarm = &data->alarms[i];

		if (alarm->enabled == false) {
			continue;
		}

		if (alarm->triggered == true) {
			continue;
		}

		if (alarm->compare != data->counter) {
			continue;
		}

		alarm->triggered = true;

		if (alarm->triggered_handler == NULL) {
			continue;
		}

		alarm->triggered_handler(dev, (size_t)i, alarm->triggered_handler_user_data);
	}
}

static void rtc_emul_update(struct k_work *work)
{
	struct rtc_emul_work_delayable *work_delayable = (struct rtc_emul_work_delayable *)work;
	const struct device *dev = work_delayable->dev;
	struct rtc_emul_data *data = (struct rtc_emul_data *)dev->data;

	k_work_schedule(&work_delayable->dwork, K_MSEC(1000));

	k_mutex_lock(&data->lock, K_FOREVER);

	data->counter++;

	rtc_emul_test_alarms(dev);

	k_mutex_unlock(&data->lock);
}

static int rtc_emul_counter_set(const struct device *dev, uint32_t counter)
{
	struct rtc_emul_data *data = (struct rtc_emul_data *)dev->data;

	k_mutex_lock(&data->lock, K_FOREVER);

	data->counter = counter;

	k_mutex_unlock(&data->lock);

	return 0;
}

static int rtc_emul_counter_get(const struct device *dev, uint32_t *counter)
{
	struct rtc_emul_data *data = (struct rtc_emul_data *)dev->data;

	k_mutex_lock(&data->lock, K_FOREVER);

	(*counter) = data->counter;

	k_mutex_unlock(&data->lock);

	return 0;
}

static int rtc_emul_alarm_config_set(const struct device *dev, size_t id,
				     const struct rtc_alarm_config *config)
{
	struct rtc_emul_data *data = (struct rtc_emul_data *)dev->data;

	/* Validate arguments */
	if (config == NULL) {
		return -EINVAL;
	}

	if (data->alarms_count <= id) {
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	data->alarms[id].compare  = config->compare;
	data->alarms[id].triggered_handler = config->handler;
	data->alarms[id].triggered_handler_user_data = config->user_data;
	data->alarms[id].triggered = false;

	k_mutex_unlock(&data->lock);
	return 0;
}

static int rtc_emul_alarm_state_get(const struct device *dev, size_t id,
				    struct rtc_alarm_state *state)
{
	struct rtc_emul_data *data = (struct rtc_emul_data *)dev->data;

	if (state == NULL) {
		return -EINVAL;
	}

	if (data->alarms_count <= id) {
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	state->compare = data->alarms[id].compare;
	state->enabled = data->alarms[id].enabled;
	state->triggered = data->alarms[id].triggered;

	k_mutex_unlock(&data->lock);
	return 0;
}

static int rtc_emul_alarm_enable(const struct device *dev, size_t id)
{
	struct rtc_emul_data *data = (struct rtc_emul_data *)dev->data;

	if (data->alarms_count <= id) {
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	data->alarms[id].enabled = true;

	k_mutex_unlock(&data->lock);
	return 0;
}

static int rtc_emul_alarm_disable(const struct device *dev, size_t id)
{
	struct rtc_emul_data *data = (struct rtc_emul_data *)dev->data;

	if (data->alarms_count <= id) {
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	data->alarms[id].enabled = false;

	k_mutex_unlock(&data->lock);
	return 0;
}

static int rtc_emul_alarm_triggered_test(const struct device *dev, size_t id)
{
	struct rtc_emul_data *data = (struct rtc_emul_data *)dev->data;
	int triggered;

	if (data->alarms_count <= id) {
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	triggered = (data->alarms[id].triggered == true) ? 1 : 0;

	k_mutex_unlock(&data->lock);
	return triggered;
}

static int rtc_emul_alarm_triggered_clear(const struct device *dev, size_t id)
{
	struct rtc_emul_data *data = (struct rtc_emul_data *)dev->data;

	k_mutex_lock(&data->lock, K_FOREVER);

	if (data->alarms_count <= id) {
		k_mutex_unlock(&data->lock);
		return -EINVAL;
	}

	data->alarms[id].triggered = false;

	k_mutex_unlock(&data->lock);
	return 0;
}

struct rtc_driver_api rtc_emul_driver_api = {
	.counter_set = rtc_emul_counter_set,
	.counter_get = rtc_emul_counter_get,
	.alarm_config_set = rtc_emul_alarm_config_set,
	.alarm_state_get = rtc_emul_alarm_state_get,
	.alarm_enable = rtc_emul_alarm_enable,
	.alarm_disable = rtc_emul_alarm_disable,
	.alarm_triggered_test = rtc_emul_alarm_triggered_test,
	.alarm_triggered_clear = rtc_emul_alarm_triggered_clear,
};

int rtc_emul_init(const struct device *dev)
{
	struct rtc_emul_data *data = (struct rtc_emul_data *)dev->data;

	k_mutex_init(&data->lock);

	data->dwork.dev = dev;
	k_work_init_delayable(&data->dwork.dwork, rtc_emul_update);

	k_work_schedule(&data->dwork.dwork, K_MSEC(1000));

	return 0;
}

#define RTC_EMUL_DEVICE(id)                                                                     \
	static struct rtc_emul_alarm rtc_emul_alarms_##id[DT_INST_PROP(id, alarms_count)];      \
												\
	struct rtc_emul_data rtc_emul_data_##id = {                                             \
		.alarms = rtc_emul_alarms_##id,                                                 \
		.alarms_count = ARRAY_SIZE(rtc_emul_alarms_##id),                               \
	};                                                                                      \
												\
	DEVICE_DT_INST_DEFINE(id, rtc_emul_init, NULL,                                          \
			      &rtc_emul_data_##id, NULL, POST_KERNEL,                           \
			      99, &rtc_emul_driver_api);

DT_INST_FOREACH_STATUS_OKAY(RTC_EMUL_DEVICE);
