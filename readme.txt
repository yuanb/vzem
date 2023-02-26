

                 VZEM
VZ200/VZ300 Emulator for windows
         Written by Guy Thomason
Email: intertek00@netspace.net.au
           Build 20230225
           February 2023

WHATS NEW
=========

- config file is now an editable text file 
- color palette is user configurable
- improvements in text load/paste
- display option for monochrome monitor
- option to inverse 256x192 hires colours


INTRODUCTION
============

VZEM is an emulator for the VZ200/VZ300 computers, also known as the 
Laser 200 / Laser 310 (Europe), Texet TX8000 (UK), and Salora Fellow (Finland).

The VZ200 was manufactured by Video Technology in Hong Kong in 1983. It was
distributed in Australia by Dick Smith electronics, and was a popular first
computer for many users due to its low cost. In 1983 the VZ200 sold for $199,
the Commodore 64 by comparison sold for around $800.

The basic specifications were:

RAM		8K (6K user ram, 2K screen ram)
ROM		16K Level I BASIC 
SOUND		Internal 2 bit piezo buzzer
TEXT		16 rows of 32 columns. 8 colour block graphics 
GRAPHICS	128x64 bitmap. 4 colours

The VZ200 had a chiclet keyboard similar to the ZX Spectrum. It was usable but
touch typing was not possible. 

The VZ300 was introduced in 1985. It had a slightly larger full travel keyboard.
Touch typing was possible but still difficult due to the small size. RAM was 
increased to 16K, plus 2K screen RAM. Video output was improved. A small switch
was located underneath the case allowing colour to be disabled, giving an even 
sharper picture on monochrome displays. The VZ300 was otherwise identical in
operation to a VZ200. 

Programs could be loaded and saved to a standard cassette player/recorder. 
Joysticks, centronics printer interface and a 16K memory expansion module were 
available. A disk drive and 64K memory expansion were released at the same time
as the VZ300.  


HARDWARE EMULATION
==================

Build 20221009 emulates the following hardware:

* Up to 34k standard RAM 
* 4mb extended RAM in 256 x 16k banks
* Mode 0 and Mode 1 graphics
* Extended graphics hardware mods 
* Internal speaker
* Joystick (via numeric keypad)
* Printer 
* Disk Drive
* Cassette Interface


EXTENDED GRAPHICS & FONTS
=========================

The "Ultra Graphics" hardware modification is emulated. This was designed by 
Matthew Sorell and documented in the "Australian Electronics Monthly" magazine
in April 1988.  This hard modification provides support for all the graphics
modes the M6847 video chip is capable of, and custom text mode fonts

To active the extended graphics modes, type the following from the emulator:

GM0	OUT 32,0	64x64		Color			1024 bytes
GM1	OUT 32,4	128x64		Monochrome		1024 bytes
GM2	OUT 32,8	128x64		Color			2048 bytes
GM3	OUT 32,12	128x96		Monochrome		1536 bytes
GM4	OUT 32,16	128x96		Color			3072 bytes
GM5	OUT 32,20	128x192		Monochrome		3072 bytes
GM6	OUT 32,24	128x192		Color			6144 bytes
GM7	OUT 32,28	256x192		Monochrome		6144 bytes

Then activate the mode using the MODE(1) command or POKE 26624,8

The VZ200 memory map can only access 2k video ram. For graphics modes that 
occupy more than 2k (eg GM7), bank switching is used. 

The byte written to port 32 is decoded via the following bits:

abcmmmbb

a	disable semigraphics, if an external font is enabled
b	inverse video
c	enable external font
mmm	graphics mode 0-7
bb	bank switch 0-3

To use an external font but still allow semigraphics, The output byte would be:

00100000 or 32. In basic, this would be OUT, 32

To use an external font and allow the full 256 custom characters, the output byte
would be:

10100000 or 160. In basic, this would be OUT, 160

Refer to the demo snapshot exttext.vz for an example 
NOTE: An external font must be loaded from the File menu. It is not part of the
config yet 

 
To activate GM7 bank 0, the output byte would be 00011100 binary or 28 decimal.
To activate GM7 bank 1, the output byte would be 00011101 binary or 29 decimal. 

For whatever bank is active, reads/writes to the 2k video memory at 7000h will
affect the screen. For GM7, pixel rows 0-63 are active for bank 0, 64-127 for 
bank 1 and 128-191 for bank 2. 

For further information read the following articles

VZ Super Graphics by Joe Leon
Ultra Graphics Adapter by Matthew Sorell.  


Support has also been added for the German Graphics Mod. 
From the main menu, select options -> extended graphics -> german.
The German modification only supports GM7, the 256x192 monochrome mode. To 
enable the mode, set bit 4 of address 30779, eg POKE 30779,8
Then activate the hi-res screen by either the MODE(1) command or POKE 26624,8

The bank switching is controlled by port 222, eg

OUT 222,0	selects bank 0
OUT 222,1	selects bank 1
OUT 222,2	selects bank 2
OUT 222,3	selects bank 3

1 bit (monochrome) bitmaps can be loaded into memory starting at 0xC000. The
bitmap *MUST* be a windows format 256x192 monochrome bitmap. Once in memory,
the bitmap can be saved to a VZ disk via BSAVE "IMAGE",C000,C7FF 


USAGE
=====

VZEM requires the following files to exist in the same directory as vzem.exe

* vzrom.v20		VZ200 rom image
* vzdos.rom		Dos rom image (if disk emulation is required)
* vzem.cfg		Configuration file

When vzem.exe is first run it will look for vzem.cfg. If this is not found VZEM
will create it with basic default configuration options.

From within the emulator, the hardware configuration can be changed from the 
"Options" menu.  To make this configuration permanent, select the "Save Config"
option.

VZEM can load a snapshot from command line, eg

VZEM -F GALAXON.VZ 

This will only work for binary snapshots. BASIC snapshots are not supported yet
due to the nature of the BASIC language initialisation when the machine starts. 

A disk image can also be mapped, eg

VZEM -D DISK1.DSK 


LOADING/SAVING 
==============

Programs can be loaded or saved as:

* Snapshots (.VZ files)
* Cassette emulation using Windows Wav files
* Disk emulation using a disk image

To save a BASIC program to a snapshot image, select File - Save VZ
To save a BASIC program to a Wav file, follow the following steps

1. In the emulator, type CSAVE “filename” but do not press enter
2. Select File - Cassette - Record
3. Enter the name of the PC file you wish to create and click SAVE
4. Press enter from the emulator

You should hear the cassette tones as the file is being saved. 

5. Once the save is complete and the tones have stopped, select
       File - Cassette - Stop  
      
It is very important to "stop" the recording just as you would if you 
physically saved a program to tape. If you do not stop, the file length is
indeterminate and the file will not be a valid wav file.

To save a program to disk, select File – Mount Disk. Select an existing disk
image, or type the name of a new file. If using an existing disk image, the 
program can be saved with the SAVE "filename" command. When creating a new disk
image, the disk must first be formatted with the INIT command. 
(See disk commands)


FULL SCREEN MODE
================

VZEM can toggle from windowed to full screen by pressing F11

CREDITS
=======

Thanks to the following people who directly supplied me with information, or 
that I have stolen ideas and code from:


Marat Fayzullin			Z80 Emulation library. His MSX emulator motived me to 
						develop a VZ200 emulator   	 


Juergen Buchmueller		Disk emulation routines and other bits and pieces


Richard Wilson			Screen timing information for use in split screen modes


OTHER RESOURCES
===============

The facebook page "VZ200 VZ300 LASER 210 LASER 310 Fans" is the current 
active user group. It contains links for documentation and downloads. The 
people there are very friendly and helpful. I'm an active member, often liking
my own posts. 