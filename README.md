# T4_EGA_VGA

Teensy 4.0 based CGA/EGA to VGA converter

![converter](/images/converter.jpeg)

_**Second version with improved shielding and protection**_

This is my poor-man's RGBi to VGA converter that can be built with just a Teensy 4.0 and a bunch of resistors. Works quite well with low resolution modes and works also but with a little less quality with high resolution modes. MDA is not supported at this moment.

To make it work you need as a bare minimum the Teensy 4.0, a VGA cable, a DE9 cable, and 25 resistors of 5 different values, so it can be made very cheaply as a DIY project on a breadboard.

The objective of this project was building the converter using only the Teensy, so it lacks any CPLD or other complex sampling and level shifting interfaces that other better converters like MCE2VGA use.

For the VGA output signal generation it uses the awesome VGA_t4 library by Jean-Marc Harvengt  https://github.com/Jean-MarcHarvengt/VGA_t4, this project wouldn't be possible without it.

Before jumping in, be sure that you have read the **current limitations** section and watch the video for an idea of its performance.


## Operation

The converter needs to be USB-powered through the Teensy micro-USB connector, from a computer port of a regular wall USB power adapter. If possible, it is better to power up the converter before connecting it to the video source.

Once the converter is powered, just connect the DE9 end to the computer/graphics card through a suitable cable to convert, and the DB15 end to the VGA monitor that will display the converted video signal. The DE9 cable mush be straight and all the pins must be connected. Try to avoid null modem or other special cables that are not straight pin-to-pin.

**Please, don't program the Teensy while connected to either the DE9 or DB15 connector.**


## Supported modes

Supported modes and some comments about each one.

http://minuszerodegrees.net/video/bios_video_modes.htm

|Mode|Resolution|VGA resolution|Scanlines|Supported|HR Switch|CPU Speed|Remarks|
|---	|---	|---	|---	|---	|---	|---	|---	|
|0x00h|320x200 greyscale|320x200|no|Resistor network*|off|600Mhz|40 columns text mode|
|0x01h|320x200 color|320x200|no|Resistor network*|off|600Mhz|40 columns text mode|
|0x02h|640x200 greyscale|640x480|yes|Level converter*|on|984Mhz|80 columns text mode|
|0x03h|640x200 color|640x480|yes|Level converter*|on|984Mhz|80 columns text mode|
|0x04h|320x200 color|320x200|no|Resistor network*|off|600Mhz|4 color CGA graphics|
|0x05h|320x200 greyscale|320x200|no|Resistor network*|off|600Mhz|4 gray CGA graphics|
|0x06h|640x200 monochrome|640x480|yes|Level converter*|on|984Mhz|CGA high res monochrome|
|0x07h|720x350 monochrome|NOT SUPPORTED|NOT SUPPORTED|NOT SUPPORTED|NOT SUPPORTED|NOT SUPPORTED|MDA monochrome mode|
|0x08h|160x200 color|UNTESTED|UNTESTED|UNTESTED|UNTESTED|UNTESTED|PCjr. mode, should work but untested|
|0x09h|320x200 color|UNTESTED|UNTESTED|UNTESTED|UNTESTED|UNTESTED|PCjr. mode, should work but untested|
|0x0Ah|640x200 color|UNTESTED|UNTESTED|UNTESTED|UNTESTED|UNTESTED|PCjr. mode, should work but untested|
|0x0Dh|320x200 color|320x200|no|Resistor network*|off|600Mhz|EGA 16 color low resolution|
|0x0Eh|640x200 color|640x480|yes|Level converter*|on|984Mhz|EGA 16 color high resolution|
|0x0Fh|640x350 monochrome|640x480|no|Level converter*|both|984Mhz|EGA monochrome ultra-high resolution|
|0x10h|640x350 color|640x480|no|Level converter*|both|984Mhz|EGA 16 color ultra-high resolution|


Notes:

**Resistor Network** This mode works with both board versions, with resistor network capture or level converter

**Level converter** This mode may work somehow with resistor network capture, but gives best results with the level converter version


## Screenshots

Some screenshots for the different supported modes. They are taken simply with a smartphone pointing to a modern IPS monitor, so the alignment isn't perfect but they can give and idea about how it works. The computer driving the display is an IBM 5160 with a Paradise EGA card.


![CAP01](/images/cap01.jpeg)
![CAP02](/images/cap02.jpeg)
![CAP03](/images/cap03.jpeg)
![CAP04](/images/cap04.jpeg)
![CAP05](/images/cap05.jpeg)
![CAP06](/images/cap06.jpeg)
![CAP07](/images/cap07.jpeg)
![CAP08](/images/cap08.jpeg)
![CAP09](/images/cap09.jpeg)
![CAP10](/images/cap10.jpeg)
![CAP11](/images/cap11.jpeg)
![CAP12](/images/cap12.jpeg)
![CAP13](/images/cap13.jpeg)
![CAP14](/images/cap14.jpeg)


## Videos

Video showing 350 lines text mode, the Checkit graphic tests, Monkey Island and Commander Keen in 320x200x16 mode and PlanetX3 in 640x350x16 mode. Yes, Monkey Island is slow, but that is because it is running in a poor IBM 5160, the converter operates always at the full 60Hz.

https://youtu.be/SZSuXO_p2-U

[![VIDEO SHOWING PERFORMANCE](https://img.youtube.com/vi/SZSuXO_p2-U/0.jpg)](https://www.youtube.com/watch?v=SZSuXO_p2-U)


## Controls.

There are 4 buttons and 1 switch that can be used to adjust the converter operation. Those are optional and not strictly required for basic operation.

* **HR ON/OFF switch  (pin 14)**. When on, the 640x200 graphic and 80 columns text modes are enabled. That is needed because the signal specs for 320x200 and 640x200 modes are exactly the same, so the converter would have a bad day to try to distinguish between them. With the switch ON, 320x200 will work the same, but with the switch OFF 320x200 will work better and the Teensy will operate at stock speed (600Mhz) and wouldn't even need extra cooling.
* **Pixel adjustment decrease (pin 6)**. Decrease 1 unit the pixel alignment. This makes the sampling start earlier and is like displacing the picture to the right
* **Pixel adjustment increase (pin 5)**. Increase 1 unit the pixel alignment. This makes the sampling start later and is like displacing the picture to the left
* **Store pixel alignment (pin 7)**. Hold this button for 2 seconds to store the current pixel value in EEPROM. The autodiscovery will be disabled for the current mode.
* **Reboot/Reset (pin 9)**. Push to reboot the Teensy. Hold for 2 seconds to erase all stored pixel alignment values and reboot, the autodiscovery will be enabled for all modes.


## Current limitations:

This is an experimental converter so some amount of pain is expected.


* **Instability**. The Teensy running at overclocking frequencies isn’t the most stable thing in the world. Some sudden reboots are to be expected. Also you will have to reboot the Teensy sometimes when changing between graphics modes. Keep it cool.

* **Noise**. sampling is not perfect and will lead to some pixel noise here and there.

* **Color smearing**. There is a limitation, especially in high res, due to the way VGA_t4 works separating the color bits between two concurrent DMA transfers. This can lead to a phase difference between the color components and visible color smearing more evident in high contrast pictures.

* **Lost pixels**. Currently some pixels are lost in the right side of the image. I think those are sacrificed to get better picture quality tweaking the DMA transfer values. This can be either OK or unacceptable to you. You can get the pixels back tuning some VGA_t4 magic numbers, but then color smearing is exacerbated.

* **Line wobbling**. There is also a problem in that the moment the VGA line start is generated  is not 100% exact and the lines can wobble a little. This is quite evident at about 2/3 of the screen horizontally.

* **Artifacts**. Sampling is affected by the ISR and some pixels and lines are lost in the process. This is very evident in the 15,7Khz modes as the VGA signal is 31,5 Khz, almost exactly double the frequency so the artifacts happen twice per line and the affected pixels in one line are more or less the same than the line before and after. This generates horizontal (entire lines lost) and diagonal (lost pixels) black lines in each individual sampled picture. Example of a single screen capture for a 15,7Khz mode:

![PATTERN15](/images/pattern_15.jpeg)


Thankfully the VGA horizontal frequency is not EXACTLY double the input horizontal frequency, because in that case the affected pixels will be always the same and a slow moving vertical black band would be always present in the picture, or if the band were around the HSYNC area you will get absolutely no picture at all.

The 21,8Khz modes are less affected and there is no visible interference pattern in the picture. Example of a single screen capture for this mode, where the small horizontal lines are the pixels lost due to the ISR firing:

![PATTERN21](/images/pattern_21.jpeg)



* **No 720px modes**. Unfortunately VGA_t4 at the moment doesn’t support any 720px mode, so MDA (720x350) is not supported. If we ever get that mode, MDA should be perfectly possible as the pixel clock is the same as EGA.

* **Needs alignment for every different card**.  The key to get a nice picture is adjusting the exact clock cycle after HSYNC where the converter starts sampling the signal. This point varies among different modes and different graphic cards. For my EGA card in CGA mode the point is quite different to a real CGA card. With a converter that operates oversampling, or for an analog monitor this is not a problem because you simply reposition the image horizontally, that is sometimes automatically done by the monitor itself. The converter tries to guess the optimal sampling point automatically looking for the first pixel in the image and remembering it afterwards, but this can fail in some cases, like:

    * There are no pixels in the first column
 
    * Noise leads to a fake first pixel detection and you lose part of the image forever
 
    * Some clever programmers generate borders around the image that are falsely interpreted as pixels. This happens for instance with Commander Keen. 

Because all of this, the user can store using the control switches a value that overrides the automatic calculation if needed.

* **Initial sync is slow**. The adapter needs to sync to signals and generate the sampling tables programmatically, so it’s not the fastest thing out there.

* **Probably won’t work well with some cards**. If some graphics card for any reason doesn’t keep a steady on-spec pixel clock, the converter will fail miserably and vertical noise bands will develop where the sampling gets out of alignment. In those cases you can try to tweak the magic numbers for the graphic modes (pixel period, etc) but that is an absolutely DIY task.


## Internal working

The converter captures the EGA/CGA signal using synchronous sampling. The program synchronizes its execution with the HSYNC signal and counts clock cycles until the arrival of each pixel, then it samples the color signals in parallel and maps each sample to its pixel using precalculated tables based in the data about the specific graphics mode that is capturing. The pixel is then color translated to the right palette and stored in a framebuffer before the arrival of the next sample.

In parallel, the VGA_t4 module manages the generation of the VGA signal from the framebuffer through the DMA resources of the Teensy using a timed ISR to initiate the DMA transfers. The DMA transfer pumps data from the framebuffer and generates the lines of the VGA signals. That happens in parallel with the sampling code execution, except for the ISR itself. 

The ISR interrupts the sampling code each time it fires, that is once or twice per line, so some synchronization code is needed to detect the signal sampling intervals affected by the ISR and discard those pixels and sometimes entire lines from the captured signal. This generates some visible artifacts, especially in fast moving or flashing scenes in the 15,7Khz modes, but it is unavoidable with this arrangement. Maybe a future Teensy would support 3 concurrent DMA channels for FlexIO pins or full 8 bit I/O for the available channels, so that sampling could be implemented also with DMA.

The Teensy needs to be overclocked to be able to sample at the insane pixel rates of high resolution modes, (16,257Mhz for the EGA 10h mode). At this moment, the 640x200 and 640x350 modes need to operate at 984Mhz. The program adjusts the Teensy speed automatically as required for the input mode detected. Low resolution modes (320x200) work well at stock speed (600Mhz).


## Building software

Simply build the sketch with Arduino with the Teensyduino addon installed https://www.pjrc.com/teensy/td_download.html and upload it to a Teensy 4.0 via USB cable. I’ve used Arduino 1.8.13 and Teensyduino 1.5.3. Be sure that you compile it for 600Mhz and “fastest” compiler optimization.


## Circuits

### VGA signal generation

On the physical side, the digital to analog conversion to generate the VGA RGB signals is done using a simple R2R resistor ladder.  VGA_t4 supports up to 8 bit color (with an experimental 12 bit mode), but to get the EGA palette you only need 6 bit color, so a 6 bit ladder with 2 bits per color channel is used. 

![R2R](/images/r2r.png)

This is the formula I used to calculate the resistor values. The objective for each color signal is getting from the 3,3V of the Teensy a 1,4V maximum level and 75 ohm output impedance, that will get converted to 0,7V when connecting to the VGA monitor and its 75 ohm terminations.

https://www.wolframalpha.com/input/?i=R2%2F%28R2%2B%282*%28R1%2F3%29%29%29+%3D+%281.4%2F3.3%29%2C+2*R1*R2%2F%283*%28R2%2B%282*R1%2F3%29%29%29%3D75

Given that calculations, I took as the final resistor values:

* 130 ohm for R2
* 255 ohm for R1 
* 510 ohm for 2R1, 

That should be close enough to the theoretical values. You can even use two 510 ohm resistors in parallel instead of the 255 ohm, or two 255 ohm in series instead of the 510 ohm, so that one less resistor value is required. This won’t win any color accuracy competition, but should be good enough.


### EGA/CGA input capture

Unfortunately, for the EGA/CGA signal sampling we need to make voltage conversion, as EGA/CGA is TTL signalling with a 5V logic level, and the Teensy 4 only supports a maximum of 3,3V in its input pins. Give them the full 5V and you will fry the poor thing. 

To overcome that there are different possible solutions, and I have selected two, one using only a resistor network and the other making use of an active voltage converter.


### Version one: Cheapo resistor divider.

![BREADBOAD](/images/breadboard.jpeg)

The true poor-man’s alternative. As the IBM CGA card line drivers are 74LS244 IC’s I calculate the minimum series resistor value from a (very optimistic, probably overestimated) 1mA maximum theoretical output current for the TTL gate. That gives for the 5V signal a maximum load of 5 Kohm. To get 3,3V from 5V through a 5K resistor divider you can use approximately a 1,5K and a 3K resistor.

This approach is cheap and easy, but it has the drawback that the resistors act like a RC network, slowing down the transitions between the high and low logical levels as the retain charge stored in the parasitic capacitance of the wires and the resistors themselves, that need to be discharged through the resistors. That flattens the signal and generates slow rising and falling edges that affect the sampling.

So with a resistor network you can expect the converter to operate like this:

* Low resolution (320X200 graphic and 40 columns text) modes work quite well. Pixel clock is around 8Mhz, slow enough for the resistor network to operate acceptably.

* High resolution text modes (80 columns) are usable with noise.    

* High resolution graphic modes work acceptable with less demanding games that doesn’t use complex graphics (like Thexder)

* High resolution graphic modes with more complex games like PlanetX3 (EGA version) work like crap. This is due to the fact that Planet X3 makes heavy use of dithering in high resolution modes, and that dithering generates a very high frequency in the color channels (16Mhz) that is basically too much for the resistor network.

If you are adventurous enough the quality can be quite improved lowering the resistor values, BUT that comes with the risk of overloading the line drivers of the graphics card or degrading the signal levels, so be warned if you try that.

Also be sure that you double check the resistor network wiring, as there is a risk the Teensy could receive the full 5V if something is wired up wrongly.


### Version two: TXS0108E voltage translator.

This is a popular bidirectional voltage translator for microcontrollers that theoretically can accept up to 55Mhz signals. It has just the 8 channels we need for the conversion (6 colors plus HSYNC and VSYNC). With this in place all the modes, high and low resolution work quite well. As it is popular it can be obtained very cheaply and you can even get it mounted in a nice ready to plug prototype board.

![TXS0108](/images/txs.jpeg)


I’ve tried other solutions, none of which was fully satisfactory:

**74HCT245 driver @5V plus a lower value (500 ohm) resistor divider.** The 74HCT can drive higher loads than the 74LS so we can significantly lower the resistor values. Works well, but too cumbersome as you need the active component PLUS the resistors to make the 5V-3.3V conversion.

**74LVC245 driver @3,3V.** Too noisy, sampling gives terrible quality.

**74FCT244 driver**, quite rubbish sampling quality, didn’t work at all.

I haven’t tried one other possible solution, that is connect the signals to the Teensy through a single current limiting series resistor per channel. That is for the most adventurous out there, I haven’t tried because I don’t want to ruin my only Teensy board, but it should be possible if the right resistor value is low enough to get a good signal and high enough not to fry the Teensy. No idea of what that value should be.


## Schematics

### Schematic for version 1, TXS0108E based capture solution.

![Schematic 1](/images/sch_1_v2.png)


### Schematic for version 2, resistor based capture solution.

![Schematic 2](/images/sch_2.png)


## Board fabrication

If you are crazy enough to make a real PCB for this, I provide the needed resources for the TXS0108E version, gerber file, bom and component placement for a popular online fabricator.


![PCB board](/images/boardv2.jpeg)


## Q&A:

**Is this better than a MCE2VGA?**

Definitely no, this is hackish and the MCE2VGA is very professional. You will have to adjust the sampling alignment all the time, the image quality is much worse and it supports less input modes and resolutions. Not to mention you can get a MCE2VGA tested, assembled and inside a nice 3D printed case for a reasonable amount of money.


**Should I use this as my main converter for my retro hardware?**

I see this as a nice weekend mini-project if you already have the Teensy, a bunch of resistors, you like it and you have no high expectations. I don’t recommend going for it as a standalone converter for general use.


**Why do you repeat so much code? Have you ever heard about functions and parameters?**

Of course I’ve heard, but in this case the code performance is absolutely critical. You have a fixed amount of clock cycles to process each sample before the next sample arrives. Some sections simply don’t work fast enough unless you use constants instead of variables, and reduce loops and logic checks to the bare minimum. And that is even more complicated by the VGA ISR firing all the time and interfering with the sampling. So I need/prefer to keep the modes all separated in code, despite 99% of that code being identical between different modes. Maybe some assembly code would help but actually it is working without needing any. Maybe that could make high resolution modes work at lower clock frequencies.


**Could this converter kill my beloved vintage graphics card/computer/monitor?**

Yes it can. I’ve not been able to test it with much hardware, I only have an IBM CGA card and a Paradise EGA card to test the converter, and with those it works flawlessly, but who knows with others.


**Could this converter kill my Teensy?**

High resolution modes work only with the Teensy overclocked at very high speeds (984Mhz), so you absolutely need to run the Teensy with some cooling in place, and there is a certain risk it will become unstable, be killed in action, catch fire, explode, or generate a mini black hole. The low resolution modes work at stock speed (600Mhz) so they are much more friendly.


**Why don’t you also use DMA for sampling, like VGA_t4 does for VGA signal generation?**

Because the DMA resources are limited and they are already taken for VGA signal generation. FlexIO1 and FlexIO2 ports are already taken and FlexIO3 doesn’t support DMA operation. Also it would be much more difficult to manage sampling alignment and it is absolutely critical for proper operation, and I suppose that you don’t have as much control over the DMA precise timings once a transfer is initiated. For the VGA signal, some out of alignment will make no difference at all but for synchronous sampling that is unacceptable.


**Why don’t you sample at higher rates? Have you heard about Nyquist?**

I think commercial FPGA based solutions sample at like 4xNyquist near 130Mhz, so they can get a very precise alignment and give a very high image quality that way, but that rates are simply out of reach for a Teensy or any microcontroller today in existence, so other strategies need to be used to make it work without specialized hardware like the MCE2VGA or Raspberry pi zero + CPLD solution do.


## Details about graphic modes definition.

If you want to tinker with the magic numbers, this is an explanation about each one.

```
//CLOCK AND SAMPLING MODES
//UNCOMMENT JUST ONE BLOCK FOR EACH MODE (200, 200_HR, 350)

//STOCK 600Mhz 200 LINES PARADISE EGA
/**/
const String NAME_200_COLOR = "Mode 320x200x16 @60";
const int MIN_CYCLES_HSYNC_200_COLOR  = 38169  ;
const int CYCLES_HSYNC_HIRES_200_COLOR = 38000  ;
const int CYCLES_FIRST_PIXEL_200_COLOR  = 7480; 
const int CYCLES_PIXEL_200_COLOR  = 84  ;
const int CYCLES_PIXEL_200_COLOR_EXACT = 83811  ;
const int CYCLES_HALF_PIXEL_200_COLOR_EXACT = 41905 ;
const int CYCLES_HALF_PIXEL_200_COLOR = 42  ;
const int CYCLES_PIXEL_200_COLOR_MAX_SKEW = 20  ;
const int CYCLES_PIXEL_200_COLOR_PRESAMPLE = 0;
const int CYCLES_2_PIXEL_200_COLOR = 168  ;
const int MAX_CYCLES_VSYNC_200_COLOR = 10020020 ;
const int MIN_CYCLES_VSYNC_200_COLOR = 10000000  ;
const int BLANK_LINES_SCREEN_200_COLOR = 36  ;
const int LINES_200_LINES = 200 ;
const int PIXELS_200_LINES = 320;
const int SCANLINES_200_LINES = 2;
const unsigned long CPU_FREQ_200_LINES  = 600000000;
const vga_mode_t VGA_MODE_200_LINES = VGA_MODE_320x480;
```

* NAME: Name for the mode that will be displayed in the OSD
* CPU_FREQ: Teensy will be configured to run at this clock speed in this mode. This value affects all the others as it has to be taken into account in their calculations
* MIN_CYCLES_HSYNC: minimum acceptable period of the HSYNC signal in clock cycles
* CYCLES_HSYNC_HIRES: cycles since last HSYNC when we will start to seek actively the following HSYNC
* CYCLES_FIRST_PIXEL: Default position in clock cycles after the last HSYNC edge where we will sample the first pixel. This value is not used if autodiscovery is enabled or a user value is stored. This is a key value and is much dependent on the particular hardware and graphic mode.
* CYCLES_PIXEL: Clock cycles between consecutive pixels. This value is rounded to an integer
* CYCLES_PIXEL_EXACT: Clock cycles between consecutive pixels measured in millicycles
* CYCLES_HALF_PIXEL: Half the clock cycles between consecutive pixels. This value is rounded to an integer
* CYCLES_HALF_PIXEL_EXACT: Half the clock cycles between consecutive pixels measured in millicycles
* CYCLES_PIXEL_MAX_SKEW: Any sample that is early or late by more than this amount of clock cycles will be discarded
* CYCLES_PIXEL_PRESAMPLE: As the sampling mechanism is released by a loop that takes itself some clock cycles to process, it’s not always possible to get accurately to the sampling point. This value allows releasing the loop earlier by the given amount of clock cycles to try to get to the sampling point more accurately.
* MAX_CYCLES_VSYNC : maximum acceptable period of the VSYNC signal in clock cycles
* MIN_CYCLES_VSYNC : minimum acceptable period of the VSYNC signal in clock cycles
* BLANK_LINES_SCREEN: Number of empty lines that will be ignored after VSYNC
* PIXELS: Pixels per line that will be captured
* SCANLINES: 1-> no scanlines; 2-> an empty line will be added in the output for each captured line
* VGA_MODE: Output mode the VGA_t4 output will be put in for this input mode
