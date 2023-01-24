/*
 * RTCC (RTC) driver for the ATMEL SAM family
 *
 * Copyright 2022 Bjarki Arge Andreasen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*************************************************************************************************/
/*                                          Dependencies                                         */
/*************************************************************************************************/
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/rtcc.h>

#include <time.h>
#include <string.h>

/*************************************************************************************************/
/*                                           Registers                                           */
/*************************************************************************************************/
#define RTCC_ATMEL_SAM_RTC_CR_OFFSET                    (0x00)
#define RTCC_ATMEL_SAM_RTC_CR_UPDTIM_OFFSET             (0x00)
#define RTCC_ATMEL_SAM_RTC_CR_UPDTIM_SIZE               (0x01)
#define RTCC_ATMEL_SAM_RTC_CR_UPDCAL_OFFSET             (0x01)
#define RTCC_ATMEL_SAM_RTC_CR_UPDCAL_SIZE               (0x01)

#define RTCC_ATMEL_SAM_RTC_MR_OFFSET                    (0x04)

#define RTCC_ATMEL_SAM_RTC_TIMR_OFFSET                  (0x08)
#define RTCC_ATMEL_SAM_RTC_TIMR_SECOND_OFFSET           (0X00)
#define RTCC_ATMEL_SAM_RTC_TIMR_SECOND_SIZE             (0X07)
#define RTCC_ATMEL_SAM_RTC_TIMR_MINUTE_OFFSET           (0X08)
#define RTCC_ATMEL_SAM_RTC_TIMR_MINUTE_SIZE             (0X07)
#define RTCC_ATMEL_SAM_RTC_TIMR_HOUR_OFFSET             (0X10)
#define RTCC_ATMEL_SAM_RTC_TIMR_HOUR_SIZE               (0X06)

#define RTCC_ATMEL_SAM_RTC_CALR_OFFSET                  (0x0C)
#define RTCC_ATMEL_SAM_RTC_CALR_CENT_OFFSET             (0x00)
#define RTCC_ATMEL_SAM_RTC_CALR_CENT_SIZE               (0x07)
#define RTCC_ATMEL_SAM_RTC_CALR_YEAR_OFFSET             (0x08)
#define RTCC_ATMEL_SAM_RTC_CALR_YEAR_SIZE               (0x08)
#define RTCC_ATMEL_SAM_RTC_CALR_MONTH_OFFSET            (0x10)
#define RTCC_ATMEL_SAM_RTC_CALR_MONTH_SIZE              (0x05)
#define RTCC_ATMEL_SAM_RTC_CALR_DAY_OFFSET              (0x15)
#define RTCC_ATMEL_SAM_RTC_CALR_DAY_SIZE                (0x03)
#define RTCC_ATMEL_SAM_RTC_CALR_DATE_OFFSET             (0x18)
#define RTCC_ATMEL_SAM_RTC_CALR_DATE_SIZE               (0x06)

#define RTCC_ATMEL_SAM_RTC_TIMALR_OFFSET                (0x10)
#define RTCC_ATMEL_SAM_RTC_TIMALR_SECOND_OFFSET         (0X00)
#define RTCC_ATMEL_SAM_RTC_TIMALR_SECOND_SIZE           (0X07)
#define RTCC_ATMEL_SAM_RTC_TIMALR_SECEN_OFFSET          (0X07)
#define RTCC_ATMEL_SAM_RTC_TIMALR_SECEN_SIZE            (0X01)
#define RTCC_ATMEL_SAM_RTC_TIMALR_MINUTE_OFFSET         (0X08)
#define RTCC_ATMEL_SAM_RTC_TIMALR_MINUTE_SIZE           (0X07)
#define RTCC_ATMEL_SAM_RTC_TIMALR_MINEN_OFFSET          (0X0F)
#define RTCC_ATMEL_SAM_RTC_TIMALR_MINEN_SIZE            (0X01)
#define RTCC_ATMEL_SAM_RTC_TIMALR_HOUR_OFFSET           (0X10)
#define RTCC_ATMEL_SAM_RTC_TIMALR_HOUR_SIZE             (0X06)
#define RTCC_ATMEL_SAM_RTC_TIMALR_HOUREN_OFFSET         (0X17)
#define RTCC_ATMEL_SAM_RTC_TIMALR_HOUREN_SIZE           (0X01)

#define RTCC_ATMEL_SAM_RTC_CALALR_OFFSET                (0x14)
#define RTCC_ATMEL_SAM_RTC_CALALR_MONTH_OFFSET          (0x10)
#define RTCC_ATMEL_SAM_RTC_CALALR_MONTH_SIZE            (0x05)
#define RTCC_ATMEL_SAM_RTC_CALALR_MTHEN_OFFSET          (0x17)
#define RTCC_ATMEL_SAM_RTC_CALALR_MTHEN_SIZE            (0x01)
#define RTCC_ATMEL_SAM_RTC_CALALR_DATE_OFFSET           (0x18)
#define RTCC_ATMEL_SAM_RTC_CALALR_DATE_SIZE             (0x06)
#define RTCC_ATMEL_SAM_RTC_CALALR_DATEEN_OFFSET         (0x1F)
#define RTCC_ATMEL_SAM_RTC_CALALR_DATEEN_SIZE           (0x01)

#define RTCC_ATMEL_SAM_RTC_SR_OFFSET                    (0x18)
#define RTCC_ATMEL_SAM_RTC_SR_ACKUPD_OFFSET             (0x00)
#define RTCC_ATMEL_SAM_RTC_SR_ACKUPD_SIZE               (0x01)
#define RTCC_ATMEL_SAM_RTC_SR_ALARM_OFFSET              (0x01)
#define RTCC_ATMEL_SAM_RTC_SR_ALARM_SIZE                (0x01)

#define RTCC_ATMEL_SAM_RTC_SCCR_OFFSET                  (0x1C)
#define RTCC_ATMEL_SAM_RTC_SCCR_ACKUPD_OFFSET           (0x00)
#define RTCC_ATMEL_SAM_RTC_SCCR_ACKUPD_SIZE             (0x01)
#define RTCC_ATMEL_SAM_RTC_SCCR_ALARM_OFFSET            (0x01)
#define RTCC_ATMEL_SAM_RTC_SCCR_ALARM_SIZE              (0x01)

#define RTCC_ATMEL_SAM_RTC_IER_OFFSET                   (0x20)
#define RTCC_ATMEL_SAM_RTC_IER_ACKEN_OFFSET             (0x00)
#define RTCC_ATMEL_SAM_RTC_IER_ACKEN_SIZE               (0x01)
#define RTCC_ATMEL_SAM_RTC_IER_ALREN_OFFSET             (0x01)
#define RTCC_ATMEL_SAM_RTC_IER_ALREN_SIZE               (0x01)

#define RTCC_ATMEL_SAM_RTC_IDR_OFFSET                   (0x24)
#define RTCC_ATMEL_SAM_RTC_IDR_ACKEN_OFFSET             (0x00)
#define RTCC_ATMEL_SAM_RTC_IDR_ACKEN_SIZE               (0x01)
#define RTCC_ATMEL_SAM_RTC_IDR_ALRDIS_OFFSET            (0x01)
#define RTCC_ATMEL_SAM_RTC_IDR_ALRDIS_SIZE              (0x01)

#define RTCC_ATMEL_SAM_RTC_IMR_OFFSET                   (0x28)
#define RTCC_ATMEL_SAM_RTC_IMR_ACKEN_OFFSET             (0x00)
#define RTCC_ATMEL_SAM_RTC_IMR_ACKEN_SIZE               (0x01)
#define RTCC_ATMEL_SAM_RTC_IMR_ALR_OFFSET               (0x01)
#define RTCC_ATMEL_SAM_RTC_IMR_ALR_SIZE                 (0x01)

#define RTCC_ATMEL_SAM_RTC_VER_OFFSET                   (0x2C)
#define RTCC_ATMEL_SAM_RTC_VER_NVTIM_OFFSET             (0x00)
#define RTCC_ATMEL_SAM_RTC_VER_NVTIM_SIZE               (0x01)

/*************************************************************************************************/
/*                                             Macros                                            */
/*************************************************************************************************/
#define RTCC_ATMEL_SAM_TO_BCD(value)                                    \
	(((((uint32_t)value) / 10) << 4) + (value % 10))

#define RTCC_ATMEL_SAM_FROM_BCD(value)                                  \
	(((value >> 4) * 10) + (value & BIT_MASK(4)))

#define RTCC_ATMEL_SAM_RTC_FIELD_GET(val, reg, field)                   \
	((val >> RTCC_ATMEL_SAM_RTC_##reg##_##field##_OFFSET) &         \
	BIT_MASK(RTCC_ATMEL_SAM_RTC_##reg##_##field##_SIZE))

#define RTCC_ATMEL_SAM_RTC_FIELD_GET_BCD(val, reg, field)               \
	RTCC_ATMEL_SAM_FROM_BCD(RTCC_ATMEL_SAM_RTC_FIELD_GET(val, reg,  \
	field))

#define RTCC_ATMEL_SAM_RTC_FIELD(reg, field, val)                       \
	((((uint32_t)val) &                                             \
	BIT_MASK(RTCC_ATMEL_SAM_RTC_##reg##_##field##_SIZE)) <<         \
	RTCC_ATMEL_SAM_RTC_##reg##_##field##_OFFSET)

#define RTCC_ATMEL_SAM_RTC_FIELD_BCD(reg, field, val)                   \
	(RTCC_ATMEL_SAM_RTC_FIELD(reg, field,                           \
	RTCC_ATMEL_SAM_TO_BCD(val)))

#define RTCC_ATMEL_SAM_RTC_BIT_MASK(reg, field)                         \
	(BIT_MASK(RTCC_ATMEL_SAM_RTC_##reg##_##field##_SIZE) <<         \
	RTCC_ATMEL_SAM_RTC_##reg##_##field##_OFFSET)

#define RTCC_ATMEL_SAM_RTC_REG_PTR(base, reg)                           \
	((uint32_t *)(base + RTCC_ATMEL_SAM_RTC_##reg##_OFFSET))

#define RTCC_ATMEL_SAM_RTC_REG(base, reg)                               \
	(*RTCC_ATMEL_SAM_RTC_REG_PTR(base, reg))

#define RTCC_ATMEL_SAM_REG(address)                                     \
	(*((uint32_t*)address))

/*************************************************************************************************/
/*                                           Definitions                                         */
/*************************************************************************************************/
#define DT_DRV_COMPAT                           atmel_sam_rtcc
#define RTCC_ATMEL_SAM_WP_REG_RTC_WP_DISABLE    (0x52544300)
#define RTCC_ATMEL_SAM_WP_REG_RTC_WP_ENABLE     (0x52544301)

/*************************************************************************************************/
/*                                             Types                                             */
/*************************************************************************************************/
typedef void (*rtcc_atmel_sam_irq_init_fn_ptr)(void);

struct rtcc_atmel_sam_config {
	/* Base memory address of ATMEL SAM RTC component */
	uint32_t base;

	/* IRQ number */
	uint32_t irq_num;

	/* Memory address of ATMEL SAM system controller write protection register */
	uint32_t wp_reg;

	/* Pointer to IRQ init function which is statically defined per instance */
	rtcc_atmel_sam_irq_init_fn_ptr irq_init_fn_ptr;
};

struct rtcc_atmel_sam_alarm {
	rtcc_alarm_triggered_handler handler;
	void *user_data;
};

struct rtcc_atmel_sam_data {
	/* Alarm data */
	struct rtcc_atmel_sam_alarm alarm;

	/* Synchronization */
	struct k_mutex lock;
	struct k_sem cr_upd_ack_sem;
};

/*************************************************************************************************/
/*                                        Static functions                                       */
/*************************************************************************************************/
static void rtcc_atmel_sam_write_protection_disable(const struct rtcc_atmel_sam_config *config)
{
	RTCC_ATMEL_SAM_REG(config->wp_reg) = RTCC_ATMEL_SAM_WP_REG_RTC_WP_DISABLE;
}

static void rtcc_atmel_sam_write_protection_enable(const struct rtcc_atmel_sam_config *config)
{
	RTCC_ATMEL_SAM_REG(config->wp_reg) = RTCC_ATMEL_SAM_WP_REG_RTC_WP_ENABLE;
}

static uint32_t rtcc_atmel_sam_mask_supported_get(void)
{
	return (RTCC_ALARM_MATCH_MASK_SECOND | RTCC_ALARM_MATCH_MASK_MINUTE |
		RTCC_ALARM_MATCH_MASK_HOUR | RTCC_ALARM_MATCH_MASK_MONTHDAY |
		RTCC_ALARM_MATCH_MASK_MONTH);
}

static bool rtcc_atmel_sam_tm_validate(const struct tm *datetime, uint32_t mask)
{
	if ((mask & RTCC_ALARM_MATCH_MASK_SECOND) &&
	    (datetime->tm_sec < 0 || datetime->tm_sec > 59)) {
		return false;
	}

	if ((mask & RTCC_ALARM_MATCH_MASK_MINUTE) &&
	    (datetime->tm_min < 0 || datetime->tm_min > 59)) {
		return false;
	}

	if ((mask & RTCC_ALARM_MATCH_MASK_HOUR) &&
	    (datetime->tm_hour < 0 || datetime->tm_hour > 23)) {
		return false;
	}

	if ((mask & RTCC_ALARM_MATCH_MASK_MONTH) &&
	    (datetime->tm_mon < 0 || datetime->tm_mon > 11)) {
		return false;
	}

	if ((mask & RTCC_ALARM_MATCH_MASK_MONTHDAY) &&
	    (datetime->tm_mday < 1 || datetime->tm_mday > 31)) {
		return false;
	}

	if ((mask & RTCC_ALARM_MATCH_MASK_YEAR) &&
	    (datetime->tm_year < 0 || datetime->tm_year > 199)) {
		return false;
	}

	return true;
}

static uint32_t rtcc_atmel_sam_timr_from_tm(const struct tm *datetime)
{
	uint32_t timr;

	timr = RTCC_ATMEL_SAM_RTC_FIELD_BCD(TIMR, SECOND, datetime->tm_sec);
	timr |= RTCC_ATMEL_SAM_RTC_FIELD_BCD(TIMR, MINUTE, datetime->tm_min);
	timr |= RTCC_ATMEL_SAM_RTC_FIELD_BCD(TIMR, HOUR, datetime->tm_hour);

	return timr;
}

static uint32_t rtcc_atmel_sam_calr_from_tm(const struct tm *datetime)
{
	uint32_t calr;
	uint32_t remaining;

	/* Set month and day of month */
	calr = RTCC_ATMEL_SAM_RTC_FIELD_BCD(CALR, DATE, datetime->tm_mday);
	calr |= RTCC_ATMEL_SAM_RTC_FIELD_BCD(CALR, MONTH, (datetime->tm_mon + 1));

	/* Set year */
	remaining = (uint32_t)(datetime->tm_year + 1900);

	calr |= RTCC_ATMEL_SAM_RTC_FIELD_BCD(CALR, CENT, (remaining / 100));
	remaining = remaining % 100;

	calr |= RTCC_ATMEL_SAM_RTC_FIELD_BCD(CALR, YEAR, remaining);

	/* Set weekday */
	calr |= RTCC_ATMEL_SAM_RTC_FIELD_BCD(CALR, DAY, (datetime->tm_wday + 1));

	return calr;
}

static uint32_t rtcc_atmel_timalr_from_tm(const struct tm *datetime, uint32_t mask)
{
	uint32_t timalr = 0;

	if (mask & RTCC_ALARM_MATCH_MASK_SECOND) {
		timalr |= RTCC_ATMEL_SAM_RTC_BIT_MASK(TIMALR, SECEN);
		timalr |= RTCC_ATMEL_SAM_RTC_FIELD_BCD(TIMALR, SECOND, datetime->tm_sec);
	}

	if (mask & RTCC_ALARM_MATCH_MASK_MINUTE) {
		timalr |= RTCC_ATMEL_SAM_RTC_BIT_MASK(TIMALR, MINEN);
		timalr |= RTCC_ATMEL_SAM_RTC_FIELD_BCD(TIMALR, MINUTE, datetime->tm_min);
	}

	if (mask & RTCC_ALARM_MATCH_MASK_HOUR) {
		timalr |= RTCC_ATMEL_SAM_RTC_BIT_MASK(TIMALR, HOUREN);
		timalr |= RTCC_ATMEL_SAM_RTC_FIELD_BCD(TIMALR, HOUR, datetime->tm_hour);
	}

	return timalr;
}

static uint32_t rtcc_atmel_calalr_from_tm(const struct tm *datetime, uint32_t mask)
{
	uint32_t calalr = 0;

	if (mask & RTCC_ALARM_MATCH_MASK_MONTH) {
		calalr |= RTCC_ATMEL_SAM_RTC_BIT_MASK(CALALR, MTHEN);
		calalr |= RTCC_ATMEL_SAM_RTC_FIELD_BCD(CALALR, MONTH, (datetime->tm_mon + 1));
	}

	if (mask & RTCC_ALARM_MATCH_MASK_MONTHDAY) {
		calalr |= RTCC_ATMEL_SAM_RTC_BIT_MASK(CALALR, DATEEN);
		calalr |= RTCC_ATMEL_SAM_RTC_FIELD_BCD(CALALR, DATE, datetime->tm_mday);
	}

	return calalr;
}

static uint32_t rtcc_atmel_sam_alarm_mask_from_timalr(uint32_t timalr)
{
	uint32_t mask = 0;

	if (timalr & RTCC_ATMEL_SAM_RTC_BIT_MASK(TIMALR, SECEN)) {
		mask |= RTCC_ALARM_MATCH_MASK_SECOND;
	}

	if (timalr & RTCC_ATMEL_SAM_RTC_BIT_MASK(TIMALR, MINEN)) {
		mask |= RTCC_ALARM_MATCH_MASK_MINUTE;
	}

	if (timalr & RTCC_ATMEL_SAM_RTC_BIT_MASK(TIMALR, HOUREN)) {
		mask |= RTCC_ALARM_MATCH_MASK_HOUR;
	}

	return mask;
}

static uint32_t rtcc_atmel_sam_alarm_mask_from_calalr(uint32_t calalr)
{
	uint32_t mask = 0;

	if (calalr & RTCC_ATMEL_SAM_RTC_BIT_MASK(CALALR, MTHEN)) {
		mask |= RTCC_ALARM_MATCH_MASK_MONTH;
	}

	if (calalr & RTCC_ATMEL_SAM_RTC_BIT_MASK(CALALR, DATEEN)) {
		mask |= RTCC_ALARM_MATCH_MASK_MONTHDAY;
	}

	return mask;
}

static void rtcc_atmel_sam_tm_from_timalr_calalr(struct tm *datetime, uint32_t mask,
						 uint32_t timalr, uint32_t calalr)
{
	memset(datetime, 0x00, sizeof(*datetime));

	if (mask & RTCC_ALARM_MATCH_MASK_SECOND) {
		datetime->tm_sec = RTCC_ATMEL_SAM_RTC_FIELD_GET_BCD(timalr, TIMALR, SECOND);
	}

	if (mask & RTCC_ALARM_MATCH_MASK_MINUTE) {
		datetime->tm_mon = RTCC_ATMEL_SAM_RTC_FIELD_GET_BCD(timalr, TIMALR, MINUTE);
	}

	if (mask & RTCC_ALARM_MATCH_MASK_HOUR) {
		datetime->tm_hour = RTCC_ATMEL_SAM_RTC_FIELD_GET_BCD(timalr, TIMALR, HOUR);
	}

	if (mask & RTCC_ALARM_MATCH_MASK_MONTHDAY) {
		datetime->tm_mday = RTCC_ATMEL_SAM_RTC_FIELD_GET_BCD(calalr, CALALR, DATE);
	}

	if (mask & RTCC_ALARM_MATCH_MASK_MONTH) {
		datetime->tm_mon = RTCC_ATMEL_SAM_RTC_FIELD_GET_BCD(calalr, CALALR, MONTH) - 1;
	}
}

static void rtcc_atmel_sam_isr(const struct device *dev)
{
	struct rtcc_atmel_sam_config *config = (struct rtcc_atmel_sam_config *)dev->config;
	struct rtcc_atmel_sam_data *data = (struct rtcc_atmel_sam_data *)dev->data;
	uint32_t sr;

	sr = RTCC_ATMEL_SAM_RTC_REG(config->base, SR);

	/* Update acknowledge */
	if (RTCC_ATMEL_SAM_RTC_FIELD_GET(sr, SR, ACKUPD)) {
		k_sem_give(&data->cr_upd_ack_sem);

		RTCC_ATMEL_SAM_RTC_REG(config->base, SCCR) =
			RTCC_ATMEL_SAM_RTC_BIT_MASK(SCCR, ACKUPD);
	}

	/* Alarm */
	if ((RTCC_ATMEL_SAM_RTC_FIELD_GET(sr, SR, ALARM)) &&
	    (data->alarm.handler != NULL)) {
		data->alarm.handler(dev, 0, data->alarm.user_data);

		RTCC_ATMEL_SAM_RTC_REG(config->base, SCCR) =
			RTCC_ATMEL_SAM_RTC_BIT_MASK(SCCR, ALARM);
	}
}

/*************************************************************************************************/
/*                                    RTCC API implementation                                    */
/*************************************************************************************************/
static int rtcc_atmel_sam_time_set(const struct device *dev, const struct tm *datetime)
{
	struct rtcc_atmel_sam_data *data = (struct rtcc_atmel_sam_data *)dev->data;

	const struct rtcc_atmel_sam_config *config =
		(const struct rtcc_atmel_sam_config *)dev->config;

	uint32_t mask_supported = rtcc_atmel_sam_mask_supported_get();

	if (rtcc_atmel_sam_tm_validate(datetime, mask_supported) == false) {
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	rtcc_atmel_sam_write_protection_disable(config);

	/* Request update */
	RTCC_ATMEL_SAM_RTC_REG(config->base, CR) = (RTCC_ATMEL_SAM_RTC_FIELD(CR, UPDTIM, 1) |
						    RTCC_ATMEL_SAM_RTC_FIELD(CR, UPDCAL, 1));

	/* Await update acknowledge */
	k_sem_take(&data->cr_upd_ack_sem, K_MSEC(1500));

	RTCC_ATMEL_SAM_RTC_REG(config->base, TIMR) = rtcc_atmel_sam_timr_from_tm(datetime);
	RTCC_ATMEL_SAM_RTC_REG(config->base, CALR) = rtcc_atmel_sam_calr_from_tm(datetime);

	RTCC_ATMEL_SAM_RTC_REG(config->base, CR) = 0;

	rtcc_atmel_sam_write_protection_enable(config);

	k_mutex_unlock(&data->lock);

	return 0;
}

static int rtcc_atmel_sam_time_get(const struct device *dev, struct tm *datetime)
{
	struct rtcc_atmel_sam_config *config = (struct rtcc_atmel_sam_config *)dev->config;
	uint32_t timr0;
	uint32_t calr0;
	uint32_t timr1;

	/* Validate time valid */
	if (RTCC_ATMEL_SAM_RTC_REG(config->base, VER) &
	    RTCC_ATMEL_SAM_RTC_BIT_MASK(VER, NVTIM)) {
		return -ENODATA;
	}

	/* Read until synchronized (registers updated async) */
	while (1) {
		timr0 = RTCC_ATMEL_SAM_RTC_REG(config->base, TIMR);
		calr0 = RTCC_ATMEL_SAM_RTC_REG(config->base, CALR);
		timr1 = RTCC_ATMEL_SAM_RTC_REG(config->base, TIMR);

		if (timr0 == timr1) {
			break;
		}
	}

	datetime->tm_sec = RTCC_ATMEL_SAM_RTC_FIELD_GET_BCD(timr0, TIMR, SECOND);
	datetime->tm_min = RTCC_ATMEL_SAM_RTC_FIELD_GET_BCD(timr0, TIMR, MINUTE);
	datetime->tm_hour = RTCC_ATMEL_SAM_RTC_FIELD_GET_BCD(timr0, TIMR, HOUR);
	datetime->tm_mday = RTCC_ATMEL_SAM_RTC_FIELD_GET_BCD(calr0, CALR, DATE);
	datetime->tm_mon = RTCC_ATMEL_SAM_RTC_FIELD_GET_BCD(calr0, CALR, MONTH) - 1;

	datetime->tm_year = RTCC_ATMEL_SAM_RTC_FIELD_GET_BCD(calr0, CALR, YEAR);
	datetime->tm_year += RTCC_ATMEL_SAM_RTC_FIELD_GET_BCD(calr0, CALR, CENT) * 100;
	datetime->tm_year -= 1900;

	datetime->tm_wday = RTCC_ATMEL_SAM_RTC_FIELD_GET_BCD(calr0, CALR, DAY) - 1;
	datetime->tm_yday = -1;
	datetime->tm_isdst = -1;

	return 0;
}

static int rtcc_atmel_sam_alarm_config_set(const struct device *dev, size_t id,
					   const struct rtcc_alarm_config *config)
{
	struct rtcc_atmel_sam_data *data = (struct rtcc_atmel_sam_data *)dev->data;
	struct rtcc_atmel_sam_config *dev_config = (struct rtcc_atmel_sam_config *)dev->config;
	uint32_t timalr;
	uint32_t calalr;
	uint32_t mask_supported;
	
	mask_supported = rtcc_atmel_sam_mask_supported_get();

	if (id != 0) {
		return -EINVAL;
	}

	if (config->mask & ~mask_supported) {
		return -EINVAL;
	}

	if (rtcc_atmel_sam_tm_validate(config->datetime, config->mask) == false) {
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	irq_disable(dev_config->irq_num);

	data->alarm.handler = config->handler;
	data->alarm.user_data = config->user_data;

	if ((config->handler == NULL) || (config->mask == 0)) {
		RTCC_ATMEL_SAM_RTC_REG(dev_config->base, IDR) =
			RTCC_ATMEL_SAM_RTC_BIT_MASK(IDR, ALRDIS);
	} else {
		RTCC_ATMEL_SAM_RTC_REG(dev_config->base, IER) =
			RTCC_ATMEL_SAM_RTC_BIT_MASK(IER, ALREN);
	}

	irq_enable(dev_config->irq_num);

	rtcc_atmel_sam_write_protection_disable(dev_config);

	timalr = rtcc_atmel_timalr_from_tm(config->datetime, config->mask);
	calalr = rtcc_atmel_calalr_from_tm(config->datetime, config->mask);

	RTCC_ATMEL_SAM_RTC_REG(dev_config->base, TIMALR) = timalr;
	RTCC_ATMEL_SAM_RTC_REG(dev_config->base, CALALR) = calalr;

	rtcc_atmel_sam_write_protection_enable(dev_config);

	k_mutex_unlock(&data->lock);

	return 0;
}

static int rtcc_atmel_sam_alarm_state_get(const struct device *dev, size_t id,
					  struct rtcc_alarm_state *state)
{
	struct rtcc_atmel_sam_data *data = (struct rtcc_atmel_sam_data *)dev->data;
	struct rtcc_atmel_sam_config *config = (struct rtcc_atmel_sam_config *)dev->config;
	uint32_t timalr;
	uint32_t calalr;

	if ((id != 0) || (state == NULL)) {
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	timalr = RTCC_ATMEL_SAM_RTC_REG(config->base, TIMALR);
	calalr = RTCC_ATMEL_SAM_RTC_REG(config->base, CALALR);

	k_mutex_unlock(&data->lock);

	state->mask = rtcc_atmel_sam_alarm_mask_from_timalr(timalr);
	state->mask |= rtcc_atmel_sam_alarm_mask_from_calalr(calalr);

	state->enabled = (state->mask == 0) ? false : true;

	rtcc_atmel_sam_tm_from_timalr_calalr(&state->datetime, state->mask, timalr, calalr);

	return 0;
}

static int rtcc_atmel_sam_alarm_enable(const struct device *dev, size_t id)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(id);

	return -ENOTSUP;
}

static int rtcc_atmel_sam_alarm_disable(const struct device *dev, size_t id)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(id);

	return -ENOTSUP;
}

static int rtcc_atmel_sam_alarm_triggered_test(const struct device *dev, size_t id)
{
	struct rtcc_atmel_sam_config *config = (struct rtcc_atmel_sam_config *)dev->config;

	if (id != 0) {
		return -EINVAL;
	}

	return (RTCC_ATMEL_SAM_RTC_REG(config->base, SR) &
		RTCC_ATMEL_SAM_RTC_BIT_MASK(SR, ALARM)) ? 1 : 0;
}

static int rtcc_atmel_sam_alarm_triggered_clear(const struct device *dev, size_t id)
{
	struct rtcc_atmel_sam_data *data = (struct rtcc_atmel_sam_data *)dev->data;
	struct rtcc_atmel_sam_config *config = (struct rtcc_atmel_sam_config *)dev->config;

	if (id != 0) {
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	RTCC_ATMEL_SAM_RTC_REG(config->base, SCCR) = RTCC_ATMEL_SAM_RTC_BIT_MASK(SCCR, ALARM);

	k_mutex_unlock(&data->lock);

	return 0;
}

static int rtcc_atmel_sam_alarm_timestamp_get(const struct device *dev, size_t id,
					 struct tm *timestamp)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(id);
	ARG_UNUSED(timestamp);

	return -ENOTSUP;
}

static int rtcc_atmel_sam_alarm_timestamp_clear(const struct device *dev, size_t id)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(id);

	return -ENOTSUP;
}

static int rtcc_atmel_sam_alarm_calibration_set(const struct device *dev, int32_t calibration)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(calibration);

	return -ENOTSUP;
}

static int rtcc_atmel_sam_alarm_calibration_get(const struct device *dev, int32_t *calibration)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(calibration);

	return -ENOTSUP;
}

/*************************************************************************************************/
/*                                      RTCC API structure                                       */
/*************************************************************************************************/
struct rtcc_driver_api rtcc_atmel_sam_driver_api = {
	.time_set = rtcc_atmel_sam_time_set,
	.time_get = rtcc_atmel_sam_time_get,
	.alarm_config_set = rtcc_atmel_sam_alarm_config_set,
	.alarm_state_get = rtcc_atmel_sam_alarm_state_get,
	.alarm_enable = rtcc_atmel_sam_alarm_enable,
	.alarm_disable = rtcc_atmel_sam_alarm_disable,
	.alarm_triggered_test = rtcc_atmel_sam_alarm_triggered_test,
	.alarm_triggered_clear = rtcc_atmel_sam_alarm_triggered_clear,
	.timestamp_get = rtcc_atmel_sam_alarm_timestamp_get,
	.timestamp_clear = rtcc_atmel_sam_alarm_timestamp_clear,
};

/*************************************************************************************************/
/*                                 RTCC device driver instance                                   */
/*************************************************************************************************/
int rtcc_atmel_sam_init(const struct device *dev)
{
	struct rtcc_atmel_sam_data *data = (struct rtcc_atmel_sam_data *)dev->data;

	const struct rtcc_atmel_sam_config *config =
		(const struct rtcc_atmel_sam_config *)dev->config;

	rtcc_atmel_sam_write_protection_disable(config);

	RTCC_ATMEL_SAM_RTC_REG(config->base, CR) = 0;
	RTCC_ATMEL_SAM_RTC_REG(config->base, MR) = 0;
	RTCC_ATMEL_SAM_RTC_REG(config->base, IDR) = UINT32_MAX;

	while (RTCC_ATMEL_SAM_RTC_REG(config->base, IMR));

	RTCC_ATMEL_SAM_RTC_REG(config->base, IER) = RTCC_ATMEL_SAM_RTC_BIT_MASK(IER, ACKEN);

	rtcc_atmel_sam_write_protection_enable(config);

	k_mutex_init(&data->lock);

	k_sem_init(&data->cr_upd_ack_sem, 0, 1);

	config->irq_init_fn_ptr();

	irq_enable(config->irq_num);

	return 0;
}

#define RTCC_ATMEL_SAM_DEVICE(id)                                               \
	void rtcc_atmel_sam_irq_init_##id(void)                                 \
	{                                                                       \
		IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority),          \
			    rtcc_atmel_sam_isr, DEVICE_DT_INST_GET(id), 0);     \
	}                                                                       \
										\
	struct rtcc_atmel_sam_config rtcc_atmel_sam_config_##id = {             \
		.base = DT_INST_REG_ADDR(id),                                   \
		.irq_num = DT_INST_IRQN(0),                                     \
		.wp_reg = DT_INST_PROP(id, wp_reg),                             \
		.irq_init_fn_ptr = &rtcc_atmel_sam_irq_init_##id,               \
	};                                                                      \
										\
	struct rtcc_atmel_sam_data rtcc_atmel_sam_data_##id;                    \
										\
	DEVICE_DT_INST_DEFINE(id, rtcc_atmel_sam_init, NULL,                    \
			      &rtcc_atmel_sam_data_##id,                        \
			      &rtcc_atmel_sam_config_##id, POST_KERNEL,         \
			      99, &rtcc_atmel_sam_driver_api);

DT_INST_FOREACH_STATUS_OKAY(RTCC_ATMEL_SAM_DEVICE);
