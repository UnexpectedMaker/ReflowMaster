# ReflowMaster

Reflow Master is my open source toaster oven reflow controller that I also sell full assembled on tindie:
https://www.tindie.com/products/13378/

![Reflow Master](http://3sprockets.com.au/um/projects/reflowmaster/Pict_01.jpg)

It's custom hardware and custom code but all open source so if you want the challenge of making one yourself or you want to hack the code, go for it!

I have a live stream build of the board here if you want to see what's involved
https://www.youtube.com/watch?v=OGQ-GZZ90oE


# ReflowMaster Design Files

Inlcluded in this reposity:
- EagleCAD schematics and Board layout files (Eagle 9.1) 
- Exported Gerber files (Gerber 274X)
- STL files for the 3D printed case
- Adrduino code

# STL files for 3D printing
I have included the STL files for the case and also the exhaust fan adapter I made for my specific toaster oven. 

**NOTE:** Do not print the exhauist fan in PLA, please use PETG or ABS as PLA warps/melts at a low temp. Also DO NOT connect the 3D printed exhaust fan directly to the metal of teh oven, it will melt and ruin your oven. You will need to add some heat resistant insulator between the metal and the 3D print. 

# Which TFT?
The TFT I am using is a 2.4" SPI TFT using the ILI9341 Driver available via...

AliExpress
http://s.click.aliexpress.com/e/bQYyZYRe

Ebay
https://rover.ebay.com/rover/1/705-53470-19255-0/1?icep_id=114&ipn=icep&toolid=20004&campid=5338252684&mpre=https%3A%2F%2Fwww.ebay.com.au%2Fitm%2F2-4-240x320-SPI-TFT-LCD-Serial-240-320-ILI9341-PCB-Adapter-SD-Card-M52%2F291549777432%3FssPageName%3DSTRK%253AMEBIDX%253AIT%26_trksid%3Dp2057872.m2749.l2649

# SAMD21 Bootloader
You will need to use a SAMD21G18 that already has an Arduino bootloader on it *before* it is put on the board. If you need to add a bootloader, you'll need an ATMEL ICE and an adapter for the chip. The cheapest adapter you can get is my SAMD21G Mangler available here...
https://www.tindie.com/products/13379/

# Hacking the code
The Reflow Master board works like an "Adafruit Feather M0" - so you'll need to have the Adafruit Cortex m0 hardware profiles installed as well as the regular Cortex m0 Arduino profiles.

![Reflow Master](http://3sprockets.com.au/um/projects/reflowmaster/Pict_03.jpg)

You can use the instructions here to install everything you need to get up and running in the Arduino IDE:
https://learn.adafruit.com/adafruit-feather-m0-basic-proto/setup

You will also need the following lobraries installed
- Spline library http://github.com/kerinin/arduino-splines
- MAX31855 library by Rob Tillaart https://github.com/RobTillaart/Arduino/tree/master/libraries/MAX31855

Plus the following libraries from Library Manager
- One Button
- Adafruit_GFX
- Adafruit_ILI9341
- FlashStorage
   
Enjoy!

# Buy me a coffee or back me on Patreon?
I love making and designing projects but sharing open source projects takes a lot of thought and time. I do it because I think it’s important to share knowledge and give back to the community like many have done before me.

If you find this project useful or want to see more open source projects like it, please consider backing me on Patreon to say thanks!

[![patreon](http://3sprockets.com.au/um/PatreonSmall.jpg)](https://www.patreon.com/unexpectedmaker)

# Unexpected Maker
http://youtube.com/c/unexpectedmaker

http://twitter.com/unexpectedmaker

https://www.facebook.com/unexpectedmaker/

https://www.instagram.com/unexpectedmaker/

https://www.tindie.com/stores/seonr/

