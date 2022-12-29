:Author: Bjarki Arge Andreasen

RTC Test suite
###############

Test RTC API implementation for RTC devices.

Overview
********

This suite is design to be portable between boards. It uses the alias
``rtc`` to designate the RTC device to test. Device pm and device pm
runtime is supported by the test suite, the RTC device will be powered
on before testing, and powered down afterwards if pm is supported.

This test suite tests the following:
* Setting and getting the counter.
* Alarms if supported by hardware.

The API implementations for alarms are also tested to verify
``-ENOTSUP`` is returned if alarms are not supported by hardware.

Setting and getting time
************************

Before each test, the time is set to a default value. This test sets a
new value, then immideately gets the time, validating that the time
gotten is equal to or a second after the set time. This allows for some
processing delay from setting to getting the time.

Testing the alarms
******************

#. Configure all alarms to trigger 10 seconds from default time
#. Validate the configuration using alarm state get API
#. Validate alarms do not report as triggered using alarm triggered API
#. Set time to default time, 10 seconds before the alarms shall trigger
#. Enable all alarms
#. Wait for 15 seconds
#. Validate all alarms triggered, and returned expected data with configured triggered event handler.
#. Validate all alarms report as triggered using alarm triggered API
#. Set time to 10 seconds before the alarms are configured to trigger
#. Wait 15 seconds
#. Validate no alarms invoked triggered event handler
#. Clear all alarms triggered event
#. Set time to 10 seconds before the alarms are configured to trigger
#. Wait 15 seconds
#. Validate all alarms triggered, and returned expected data with configured triggered event handler.
