Reflow Master Arduino Code
==========================

In this repository you'll find the Reflow Master code for Arduino in 2 different versions:

Reflow Master V1 and Reflow Master V2

Reflow Master V2 HISTORY:
-------------------------

- 19/12/2020 v2.00  
                    - Initial release.
- 20/12/2020 v2.01  
                    - Increased max bake time to 3 hours
                    - Added long press for Bake Time & Temp adjustment to quickly change values, clamped at max, so it won't loop
                    - Forgot to hookup minBakeTemp, minBakeTime, maxBakeTemp, maxBakeTemp variables to buttons
                    - Oven was not turned off correctly after the bake ended
- 20/12/2020 v2.02
                    - Prevent going into the bake menu when there is a TC error
                    - Set FAN to Off by default in settings
                    - Fixed some incorrect comments 
                    - Made TC error more visible on menu screen


Reflow Master V1 HISTORY:
-------------------------

- 01/08/2018 v1.0   
                    - Initial release.
- 13/08/2018 v1.01  
                    - Settings UI button now change to show SELECT or CHANGE depending on what is selected
- 27/08/2018 v1.02  
                    - Added tangents to the curve for ESP32 support, Improved graph curves, fixed some UI glitches, made end graph time value be the end profile time
- 28/08/2018 v1.03  
                    - Added some graph smoothing
- 20/05/2019 v1.04  
                    - Increased max curve to support profiles up to 8mins
                    - Added fan on time after reflow for cooldown settings
                    - Added extra profile for Ju Feng Medium temp paste
- 09/07/2019 v1.05  
                    - Fixed some bugs, Thanks Tablatronix!
- 16/09/2019 v1.06  
                    - Fixed probe offset temp not changing in settings
- 02/07/2020 v1.07  
                    - Cleaned up some Fan control and tracking code
                    - Cleaned up some debug messages
- 13/07/2020 v1.08  
                    - Fixed bug in DEBUG mode


Open Source License
-------------------

These files are released as open source under the MIT license. Please review the license before using these files in your own projects to understand your obligations.

Support Unexpected Maker
------------------------

I love designing, making and releasing our projects as open source. I do it because I believe it’s important to share knowledge and give back to the community, like many have done before me. It helps us all learn and grow.

That said, a lot of time, effort and finances have gone into designing and releasing these files, so please consider supporting me and following me on social media

If you'd like to support me on my journey, please consider buying one of my products on tindie

https://www.tindie.com/stores/seonr/

or at
https://unexpectedmaker.com/shop

Or become a Patron:

https://www.patreon.com/unexpectedmaker


Unexpected Maker
===================
http://youtube.com/c/unexpectedmaker

http://twitter.com/unexpectedmaker

https://www.facebook.com/unexpectedmaker/

https://www.instagram.com/unexpectedmaker/
