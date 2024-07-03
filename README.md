# ReflowMaster

ReflowMaster is my original open source toaster oven reflow controller that I used to sell on tindie:
https://www.tindie.com/products/13378/

### This project is not compatible with ReflowMasterPro

![Reflow Master](http://3sprockets.com.au/um/projects/reflowmaster/Pict_01.jpg)

It's custom hardware and custom code but all open source so if you want the challenge of making one yourself or you want to hack the code, go for it!

I have a live stream build of the board here if you want to see what's involved
https://www.youtube.com/watch?v=OGQ-GZZ90oE


# ReflowMaster Design Files

Included in this repository:
- EagleCAD schematics and Board layout files (Eagle 9.1) 
- Exported Gerber files (Gerber 274X)
- STL files for the 3D printed case
- Adrduino code

# Open Source License
The hardware design files are released as open source under the CERN license. Please review the license before using these files in your own projects to understand your obligations.

# STL files for 3D printing
I have included the STL files for the case and also the exhaust fan adapter I made for my specific toaster oven. 

**NOTE:** Do not print the exhaust fan in PLA, please use PET-G or ABS as PLA warps/melts at a low temp. Also DO NOT connect the 3D printed exhaust fan directly to the metal of teh oven, it will melt and ruin your oven. You will need to add some heat resistant insulator between the metal and the 3D print. 

# Which TFT?
The TFT I am using is a 2.4" SPI TFT using the ILI9341 Driver available via...

AliExpress
https://s.click.aliexpress.com/e/_DevvrLr

Ebay
https://rover.ebay.com/rover/1/705-53470-19255-0/1?icep_id=114&ipn=icep&toolid=20004&campid=5338252684&mpre=https%3A%2F%2Fwww.ebay.com.au%2Fitm%2F2-4-240x320-SPI-TFT-LCD-Serial-240-320-ILI9341-PCB-Adapter-SD-Card-M52%2F291549777432%3FssPageName%3DSTRK%253AMEBIDX%253AIT%26_trksid%3Dp2057872.m2749.l2649

# SAMD21 Bootloader
If you build your own Reflow Master, you will need to use a SAMD21G18 that already has an Arduino bootloader on it *before* it is put on the board. If you need to add a bootloader, you'll need an ATMEL ICE and an adapter for the chip. The cheapest adapter you can get is my SAMD21G Mangler available here...
https://www.tindie.com/products/13379/

# Updating firmware & Adding profiles - for Version 2.0.0
You can flash your Reflow Master with the Arduino IDE using the code provided above.

If you wish to add or change any of the reflow profiles, you need to do so in the code and re-flash the changes to your Reflow Master.

The Reflow Master board works like an "Adafruit Feather M0" - so you'll need to have the Adafruit Cortex m0 hardware profiles installed as well as the regular Cortex m0 Arduino profiles.

![Reflow Master](http://3sprockets.com.au/um/projects/reflowmaster/Pict_03.jpg)

You can use the instructions here to install everything you need to get up and running in the Arduino IDE:
https://learn.adafruit.com/adafruit-feather-m0-basic-proto/setup

You will also need the following libraries from Library Manager
- One Button
- Adafruit_GFX
- Adafruit_ILI9341

Once you have everything you need, you can download the code from this repo and put it into the Arduino folder where your sketches are stored, load the sketch, select Adafruit Feather M0 from the boards list, plug in your RM and turn it on, and select the correct port from the ports list and flash.
   
The easiest way to put a new profile into the code is to change one of the existing profiles, by altering the values in it's class initialiser. Information of what each value is is available in the ReflowMasterProfile.h file:
https://github.com/UnexpectedMaker/ReflowMaster/blob/master/Code/Reflow_Master_v2/ReflowMasterProfile.h
   
Enjoy!

# Support Unexpected Maker

I love designing, making and releasing my projects as open source. I do it because I believe it’s important to share knowledge and give back to the community, like many have done before me. It helps us all learn and grow.

That said, a lot of time, effort and finances have gone into designing and releasing these files, so please consider supporting me by buying some of my products:

https://unexpectedmaker.com/shop

https://www.tindie.com/stores/seonr/

Or by becoming a Patron:

https://www.patreon.com/unexpectedmaker

# Join my discord server
https://discord.com/invite/xAHpApP


# Unexpected Maker
https://unexpectedmaker.com

http://youtube.com/unexpectedmaker

http://twitter.com/unexpectedmaker

https://www.facebook.com/unexpectedmaker/

https://www.instagram.com/unexpectedmaker/
