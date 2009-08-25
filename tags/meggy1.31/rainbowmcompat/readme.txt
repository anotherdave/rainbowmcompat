RainbowMCompat is a modified version of the MeggyJrSimple API that runs on the 
Rainbowduino board from Seeedstudio (http://www.seeedstudio.com/blog/?page_id=187).

It compiles and runs all the MeggyJr programs I have tried. Of course the 
Rainbowduino lacks sound and input hardware so the games are not playable but 
programs like plasma, binary clock, and text scrollers work nicely on it.

To install:

Install the MeggyJr RGB library from http://code.google.com/p/meggy-jr-rgb/ in 
the Arduino IDE environment under hardware/libraries/MeggyJr.

Copy the included files from RainbowMCompat over the top of the existing MeggyJr 
files:

   MeggyJr.cpp
   MeggyJrSimple.h

and new file
   rainbow.h
   
Try the examples from the Arduino examples menu under File, Examples, MeggyJr. 
There are other programs and a programming manual available from the above link.