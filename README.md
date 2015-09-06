# USB_CHARGER_TESTER
Arduino based USB Charger Quality Tester - A war on cheap chargers


                    Sketch/schematic by Ben Duffy,
     http://microsoldering.com.au,geelongmicrosoldering@gmail.com

         This code is in developement. It is likely to have bugs.
         It is not optimised, and a web of horrible coding ideas.
           I eventually want to be able to measure current too.

Features:
---------
  -RAPDILY CHECKS FOR VOLTAGE CHANGES
  -REPORTS NOISE P-P (RANGE)
  -10mV Accuracy
  -WORKS WITH SERIAL AND 16X2 LCD
  -TESTS CHARGERS WITH AND WITHOUT LOAD
  -TEMPERATURE TESTING AND ALARM
  -GOOD AND BAD LED's
  -AUDIBLE TONES VIA PEIZO
  -BATTERY MONITORING AND PWR SAVING 
  -HIGHLY CUSTOMISABLE WITH VARIABLES
  -CAN BE CALIBRATED (to around 10mV)


Minor Bugs:
----------- 
-Some kind of loop bug exists right now, so starting via serial (at the press a key prompt via serial monitor)
will cause an infinite loop. Use the Start Button instead (connect startpin to gnd momentarily)

-A bug exists that randomly causes test not to start after periods of inactivity
(everything else happens as expected when button is pressed but test does not start. Reset fixes)

-The time between power save reminders takes longer than expected

TO DO:
------
-measure current by measuring voltage on shunt resistor on load return

-lcd final report output is not finished

-Need to finish temp monitor code at end of loop. does not report min/max temp to user in final report
otherwise warnings and alarms are working.

-Optimise the code in many ways. Low priority. Ill do it when i have no memory left or have to for some reason

