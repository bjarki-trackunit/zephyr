.. _rtc_api:

RTC
###

Overview
********

A real-time clock tracks the date and time using an internal calendar.
The RTC is usually optimized for low energy consumption and is usually
kept running even when the system is in a low power state.

RTCs usually contain one or more alarms which can be configured to
trigger at a given time. These alarms are commonly used to wake up the
system from a low power state.

Configuration Options
*********************

Related configuration options:

* :kconfig:option:`CONFIG_RTC`

API Reference
*************

.. doxygengroup:: rtc_interface
