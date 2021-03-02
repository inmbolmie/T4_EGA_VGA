/* Teensy 4.0 EGA/CGA to VGA CONVERTER
    © 2021 Inmbolmie inmbolmie [at] gmail [dot] com
    Distributed under the GNU GPL V3.0 license
    Based on the VGA_t4 library by J-M Harvengt https://github.com/Jean-MarcHarvengt/VGA_t4
*/


#include <Arduino.h>
#include "VGA_t4.h"
#include <EEPROM.h>

//If defined, generates debug output to the serial console. There is a similar definition in VGA_t4.c
#define DEBUG
#undef DEBUG

//If defined, autodiscovery of CYCLES_FIRST_PIXEL values will be enabled.
//If undefined, the default value specified for each mode in its definition will be used instead.
#define AUTODISCOVERY

//Objects for VGA_t4
static VGA_T4 vga;
static int fb_width, fb_height;

//Stuff to reboot the Teensy
#define CPU_RESTART_ADDR (uint32_t *)0xE000ED0C
#define CPU_RESTART_VAL 0x5FA0004
#define CPU_RESTART (*CPU_RESTART_ADDR = CPU_RESTART_VAL);


//Color translation tables ID's
const int CGA = 1;
const int EGA = 0;

//Color translation tables, from CGA/EGA to VGA_T4 (limited to 6 bits)
const byte pixelTranslation[ 2 ][ 64 ] =

  //color byte is: RRxBBxGG

{ //EGA B1  B0  G1  G0  R1  R0                                                                                                                  BROWN
  { /*000000*/ 0b00000000, /*000001*/ 0b01000000, /*000010*/ 0b10000000, /*000011*/ 0b11000000, /*000100*/ 0b00000001, /*000101*/ 0b01000001, /*000110*/ 0b10000001, /*000111*/ 0b11000001,
    /*001000*/ 0b00000001, /*001001*/ 0b01000010, /*001010*/ 0b10000010, /*001011*/ 0b11000000, /*001100*/ 0b00000011, /*001101*/ 0b01000011, /*001110*/ 0b10000011, /*001111*/ 0b11000011,
    /*010000*/ 0b00001000, /*010001*/ 0b01001000, /*010010*/ 0b10000001, /*010011*/ 0b11001000, /*010100*/ 0b00001001, /*010101*/ 0b01001001, /*010110*/ 0b10001001, /*010111*/ 0b11001001,
    /*011000*/ 0b00001010, /*011001*/ 0b01001010, /*011010*/ 0b10001010, /*011011*/ 0b11001010, /*011100*/ 0b00001011, /*011101*/ 0b01001011, /*011110*/ 0b10001011, /*011111*/ 0b11001011,
    /*100000*/ 0b00010000, /*100001*/ 0b01010000, /*100010*/ 0b10010000, /*100011*/ 0b11010000, /*100100*/ 0b00010000, /*100101*/ 0b01010001, /*100110*/ 0b10010001, /*100111*/ 0b11010001,
    /*101000*/ 0b00010010, /*101001*/ 0b01010001, /*101010*/ 0b10010010, /*101011*/ 0b11010010, /*101100*/ 0b00010011, /*101101*/ 0b01010011, /*101110*/ 0b10010011, /*101111*/ 0b11010011,
    /*110000*/ 0b00011000, /*110001*/ 0b01011000, /*110010*/ 0b10011000, /*110011*/ 0b11011000, /*110100*/ 0b00011001, /*110101*/ 0b01011001, /*110110*/ 0b10011001, /*110111*/ 0b11011001,
    /*111000*/ 0b00110010, /*111001*/ 0b01011010, /*111010*/ 0b10011010, /*111011*/ 0b11011010, /*111100*/ 0b00011011, /*111101*/ 0b01011011, /*111110*/ 0b10011011, /*111111*/ 0b11011011
  },


  //CGA    B XX  G   I R NC
  //                                                              <---------LO INT-----------   ---------HIGH INT------------>
  {
    /*000000*/ 0b00000000, /*XXXXXX*/ 0b00000000, /*000010*/ 0b10000000, /*XXXXXX*/ 0b00000000, /*000100*/ 0b01001001, /*XXXXXX*/ 0b00000000, /*000110*/ 0b11001001, /*XXXXXX*/ 0b00000000,
    /*001000*/ 0b00000010, /*XXXXXX*/ 0b00000000, /*001010B*/0b10000001, /*XXXXXX*/ 0b00000000, /*001100*/ 0b01001011, /*XXXXXX*/ 0b00000000, /*001110*/ 0b11001011, /*XXXXXX*/ 0b00000000,
    /*010000*/ 0b01001001, /*XXXXXX*/ 0b00000000, /*XXXXXX*/ 0b00000000, /*XXXXXX*/ 0b00000000, /*010100*/ 0b01001001, /*XXXXXX*/ 0b00000000, /*010110*/ 0b11000000, /*XXXXXX*/ 0b00000000,
    /*XXXXXX*/ 0b00000000, /*XXXXXX*/ 0b00000000, /*XXXXXX*/ 0b00000000, /*XXXXXX*/ 0b00000000, /*011100*/ 0b00000011, /*XXXXXX*/ 0b00000000, /*011110*/ 0b11000011, /*XXXXXX*/ 0b00000000,
    /*100000*/ 0b00010000, /*XXXXXX*/ 0b00000000, /*100010*/ 0b10010000, /*XXXXXX*/ 0b00000000, /*100100*/ 0b01011001, /*XXXXXX*/ 0b00000000, /*100110*/ 0b11011001, /*XXXXXX*/ 0b00000000,
    /*101000*/ 0b00010010, /*XXXXXX*/ 0b00000000, /*101010*/ 0b10010010, /*XXXXXX*/ 0b00000000, /*101100*/ 0b01011011, /*XXXXXX*/ 0b00000000, /*101110*/ 0b11011011, /*XXXXXX*/ 0b00000000,
    /*XXXXXX*/ 0b00000000, /*XXXXXX*/ 0b00000000, /*XXXXXX*/ 0b00000000, /*XXXXXX*/ 0b00000000, /*110100*/ 0b00011000, /*XXXXXX*/ 0b00000000, /*110110*/ 0b11011000, /*XXXXXX*/ 0b00000000,
    /*XXXXXX*/ 0b00000000, /*XXXXXX*/ 0b00000000, /*XXXXXX*/ 0b00000000, /*XXXXXX*/ 0b00000000, /*111100*/ 0b00011011, /*XXXXXX*/ 0b00000000, /*111110*/ 0b11011011, /*XXXXXX*/ 0b00000000
  }
};

//Some colors for the OSD
#define BLUE       VGA_RGB(0, 0, 170)
#define LIGHT_BLUE VGA_RGB(0, 136, 255)
#define BLACK       VGA_RGB(0, 0, 0)

//Registers for direct input capture
#define IMXRT_GPIO6_DIRECT  (*(volatile uint32_t *)0x42000000)
#define IMXRT_GPIO9_DIRECT  (*(volatile uint32_t *)0x4200C000)
#define IMXRT_GPIO7_DIRECT  (*(volatile uint32_t *)0x42004000)

#define IO_BLOCK_A (IMXRT_GPIO7_DIRECT & 0b00000000000000000000000000000111)
#define IO_BLOCK_B ((IMXRT_GPIO9_DIRECT & 0b00000000000000000000000001110000))
#define IO_BLOCK_C ((IMXRT_GPIO6_DIRECT & 0b00000000000011110000000000000000))
#define IO_BLOCK_D ((IMXRT_GPIO6_DIRECT & 0b00001111110000000000000000000000))

//INPUT:
// IMXRT_GPIO6_DIRECT PINS
// 0b00001111110011110000000000000000
// 21, 20,  23, 22, 16, 17,   15, 14, 18, 19
// B1  B0   G1  G0  R1  R0   XX  XX  VS  HS    EGA
// B   NC   G   I   R   NC   XX  XX  VS  HS    CGA

//OUTPUT:
//4 3 2 11 12 10

//#define PIN_R0 ((IMXRT_GPIO6_DIRECT & 0b00001111110011110000000000000000) )
//#define PIN_R1 ((IMXRT_GPIO6_DIRECT & 0b00001111110011110000000000000000) )
//#define PIN_G1 ((IMXRT_GPIO6_DIRECT & 0b00001111110011110000000000000000) )
//#define PIN_B1 ((IMXRT_GPIO6_DIRECT & 0b00001111110011110000000000000000) )
//#define PIN_G0I ((IMXRT_GPIO6_DIRECT & 0b00001111110011110000000000000000) )
#define PINS_COLOR ((IMXRT_GPIO6_DIRECT   & 0b00001111110000000000000000000000) >> 22)
//#define PIN_HS ((IMXRT_GPIO6_DIRECT     & 0b00000000000000010000000000000000) >> 16)
//#define PIN_VS ((IMXRT_GPIO6_DIRECT     & 0b00000000000000100000000000000000) >> 17)

//Pin assignments
const int PIN_DEBUG = 1;
const int PIN_HS = 19;
const int PIN_VS = 18;
const int PIN_HR = 14;
const int PIN_RESET = 9;
const int PIN_PLUS = 6;
const int PIN_MINUS = 5;
const int PIN_STORE = 7;

//States for state machine
const int STATE_WAIT_HSYNC = 0;
const int STATE_WAIT_FIRST_PIXEL = 1;
const int STATE_READING_PIXELS = 2;
const int STATE_WAIT_VSYNC = 3;

//Polarities
const int POSITIVE = 1;
const int NEGATIVE = 0;

//Graphic modes stuff
int currentMode = 0;
vga_mode_t vgaMode = -1;
const int MODE_350_MONO = 1;
const int MODE_200_COLOR = 2;
const int MODE_350_COLOR = 3;
const int MODE_200_COLOR_HR = 4;

//EEPROM locations to store first pixel settings
const int EEPROM_CYCLES_FIRST_PIXEL_MODE_350_MONO = 0;
const int EEPROM_CYCLES_FIRST_PIXEL_MODE_200_COLOR = 2;
const int EEPROM_CYCLES_FIRST_PIXEL_MODE_350_COLOR = 4;
const int EEPROM_CYCLES_FIRST_PIXEL_MODE_200_COLOR_HR = 6;

const int EEPROM_CYCLES_FIRST_PIXEL_MODE_350_MONO_USER_DEFINED = 32;
const int EEPROM_CYCLES_FIRST_PIXEL_MODE_200_COLOR_USER_DEFINED = 34;
const int EEPROM_CYCLES_FIRST_PIXEL_MODE_350_COLOR_USER_DEFINED = 36;
const int EEPROM_CYCLES_FIRST_PIXEL_MODE_200_COLOR_HR_USER_DEFINED = 38;


//Magic numbers for input modes capture
String NAME = "";
int MIN_CYCLES_HSYNC;
int CYCLES_HSYNC_HIRES;
int CYCLES_FIRST_PIXEL;
int CYCLES_PIXEL;
int CYCLES_PIXEL_EXACT;
int CYCLES_HALF_PIXEL_EXACT;
int CYCLES_HALF_PIXEL;
int CYCLES_PIXEL_MAX_SKEW;
int CYCLES_PIXEL_PRESAMPLE;
int MAX_CYCLES_VSYNC;
int MIN_CYCLES_VSYNC;
int BLANK_LINES_SCREEN;
int LINES;
int PIXELS;
int SCANLINES ;
unsigned long CPU_FREQ;
vga_mode_t VGA_MODE;



//CLOCK AND SAMPLING MODES
//UNCOMMENT JUST ONE BLOCK FOR EACH MODE (200, 200HR, 350)

//STOCK 600Mhz 200 LINES PARADISE EGA
/**/
const String NAME_200_COLOR = "Mode 320x200x16 @60";
const int MIN_CYCLES_HSYNC_200_COLOR  = 38169  ;
const int CYCLES_HSYNC_HIRES_200_COLOR = 38000  ;
const int CYCLES_FIRST_PIXEL_200_COLOR  = 4827;
const int CYCLES_PIXEL_200_COLOR  = 84  ;
const int CYCLES_PIXEL_200_COLOR_EXACT = 83811  ;
const int CYCLES_HALF_PIXEL_200_COLOR_EXACT = 41905 ;
const int CYCLES_HALF_PIXEL_200_COLOR = 42  ;
const int CYCLES_PIXEL_200_COLOR_MAX_SKEW = 10  ;
const int CYCLES_PIXEL_200_COLOR_PRESAMPLE = 20;
const int MAX_CYCLES_VSYNC_200_COLOR = 10020020 ;
const int MIN_CYCLES_VSYNC_200_COLOR = 10000000  ;
const int BLANK_LINES_SCREEN_200_COLOR = 36;
const int LINES_200_LINES = 200;
const int PIXELS_200_LINES = 320;
const int SCANLINES_200_LINES = 2;
const unsigned long CPU_FREQ_200_LINES  = 600000000;
const vga_mode_t VGA_MODE_200_LINES = VGA_MODE_320x480;



//600Mhz 200 LINES REAL CGA
/*
  const String NAME_200_COLOR = "Mode 320x200x16 @60";
  const int MIN_CYCLES_HSYNC_200_COLOR  = 38169  ;
  const int CYCLES_HSYNC_HIRES_200_COLOR = 38000  ;
  const int CYCLES_FIRST_PIXEL_200_COLOR  = 7380; //7474  ; //7459
  const int CYCLES_PIXEL_200_COLOR  = 84  ;
  const int CYCLES_PIXEL_200_COLOR_EXACT = 83811  ;
  const int CYCLES_HALF_PIXEL_200_COLOR_EXACT = 41905 ;
  const int CYCLES_HALF_PIXEL_200_COLOR = 42  ;
  const int CYCLES_PIXEL_200_COLOR_MAX_SKEW = 20  ;
  const int CYCLES_PIXEL_200_COLOR_PRESAMPLE = 0;
  const int MAX_CYCLES_VSYNC_200_COLOR = 10020020 ;
  const int MIN_CYCLES_VSYNC_200_COLOR = 10000000  ;
  const int BLANK_LINES_SCREEN_200_COLOR = 38  ;
  const int LINES_200_LINES = 200 ;
  const int PIXELS_200_LINES = 320;
  const int SCANLINES_200_LINES = 2;
  const unsigned long CPU_FREQ_200_LINES  = 600000000;
  const vga_mode_t VGA_MODE_200_LINES = VGA_MODE_320x480;
*/

//STOCK 1008Mhz 200 LINES PARADISE EGA
/*
  const String NAME_200_COLOR = "Mode 320x200x16 @60";
  const int MIN_CYCLES_HSYNC_200_COLOR  = 64123  ;
  const int CYCLES_HSYNC_HIRES_200_COLOR = 63800  ;
  const int CYCLES_FIRST_PIXEL_200_COLOR  = 12954  ; //12391
  const int CYCLES_PIXEL_200_COLOR  = 141  ;
  const int CYCLES_PIXEL_200_COLOR_EXACT = 140802  ;
  const int CYCLES_HALF_PIXEL_200_COLOR_EXACT = 70401 ;
  const int CYCLES_HALF_PIXEL_200_COLOR = 70 ;
  const int CYCLES_PIXEL_200_COLOR_MAX_SKEW = 20  ;
  const int CYCLES_PIXEL_200_COLOR_PRESAMPLE = 0;
  const int MAX_CYCLES_VSYNC_200_COLOR = 16833634 ;
  const int MIN_CYCLES_VSYNC_200_COLOR = 16800000  ;
  const int BLANK_LINES_SCREEN_200_COLOR = 36 -2 ;
  const int LINES_200_LINES = 200;
  const int PIXELS_200_LINES = 320;
  const int SCANLINES_200_LINES = 2;
  const vga_mode_t VGA_MODE_200_LINES = VGA_MODE_320x480;
  const unsigned long CPU_FREQ_200_LINES = ;

*/


//1008Mhz 200 LINES HR PARADISE EGA
/*
  const String NAME_200_COLOR_HR = "Mode 640x200x16 @60";
  const int MIN_CYCLES_HSYNC_200_COLOR_HR  = 64123  ;
  const int CYCLES_HSYNC_HIRES_200_COLOR_HR = 63500  ;
  const int CYCLES_FIRST_PIXEL_200_COLOR_HR  = 12425;// 12391  ; //12672
  const int CYCLES_PIXEL_200_COLOR_HR  = 70  ;
  const int CYCLES_PIXEL_200_COLOR_HR_EXACT = 70401  ;
  const int CYCLES_HALF_PIXEL_200_COLOR_HR_EXACT = 35200 ;
  const int CYCLES_HALF_PIXEL_200_COLOR_HR = 35  ;
  const int CYCLES_PIXEL_200_COLOR_HR_MAX_SKEW = 15 ;
  const int CYCLES_PIXEL_200_COLOR_HR_PRESAMPLE = 5;
  const int MAX_CYCLES_VSYNC_200_COLOR_HR = 16833634 ;
  const int MIN_CYCLES_VSYNC_200_COLOR_HR = 16816817  ;
  const int BLANK_LINES_SCREEN_200_COLOR_HR = 36 ;
  const int LINES_200_LINES_HR = 204;
  const int PIXELS_200_LINES_HR = 640;
  const int SCANLINES_200_LINES_HR = 2;
  const unsigned long CPU_FREQ_200_LINES_HR = 1008000000;
  const vga_mode_t VGA_MODE_200_LINES_HR = VGA_MODE_640x480;
*/

//984Mhz 200 LINES HR PARADISE EGA
/**/
const String NAME_200_COLOR_HR = "Mode 640x200x16 @60";
const int MIN_CYCLES_HSYNC_200_COLOR_HR  = 62597  ;
const int CYCLES_HSYNC_HIRES_200_COLOR_HR = 62000  ;
const int CYCLES_FIRST_PIXEL_200_COLOR_HR  = 7807 ;
const int CYCLES_PIXEL_200_COLOR_HR  = 69  ;
const int CYCLES_PIXEL_200_COLOR_HR_EXACT = 68725  ;
const int CYCLES_HALF_PIXEL_200_COLOR_HR_EXACT = 34362 ;
const int CYCLES_HALF_PIXEL_200_COLOR_HR = 34  ;
const int CYCLES_PIXEL_200_COLOR_HR_MAX_SKEW = 10;
const int CYCLES_PIXEL_200_COLOR_HR_PRESAMPLE = 20;
const int MAX_CYCLES_VSYNC_200_COLOR_HR = 16432833 ;
const int MIN_CYCLES_VSYNC_200_COLOR_HR = 16400000  ;
const int BLANK_LINES_SCREEN_200_COLOR_HR = 36;
const int LINES_200_LINES_HR = 200;
const int PIXELS_200_LINES_HR = 640;
const int SCANLINES_200_LINES_HR = 2;
const unsigned long CPU_FREQ_200_LINES_HR = 984000000;
const vga_mode_t VGA_MODE_200_LINES_HR = VGA_MODE_640x480;




//1008Mhz 200 LINES HR REAL CGA
/*
  const String NAME_200_COLOR_HR = "Mode 640x200x16 @60";
  const int MIN_CYCLES_HSYNC_200_COLOR_HR  = 64123  ;
  const int CYCLES_HSYNC_HIRES_200_COLOR_HR = 63500  ;
  const int CYCLES_FIRST_PIXEL_200_COLOR_HR  = 11805; // 12391  ; //12672
  const int CYCLES_PIXEL_200_COLOR_HR  = 70  ;
  const int CYCLES_PIXEL_200_COLOR_HR_EXACT = 70401  ;
  const int CYCLES_HALF_PIXEL_200_COLOR_HR_EXACT = 35200 ;
  const int CYCLES_HALF_PIXEL_200_COLOR_HR = 35  ;
  const int CYCLES_PIXEL_200_COLOR_HR_MAX_SKEW = 15 ;
  const int CYCLES_PIXEL_200_COLOR_HR_PRESAMPLE = 5;
  const int MAX_CYCLES_VSYNC_200_COLOR_HR = 16833634 ;
  const int MIN_CYCLES_VSYNC_200_COLOR_HR = 16816817  ;
  const int BLANK_LINES_SCREEN_200_COLOR_HR = 36 ;
  const int LINES_200_LINES_HR = 204;
  const int PIXELS_200_LINES_HR = 640;
  const int SCANLINES_200_LINES_HR = 2;
  const unsigned long CPU_FREQ_200_LINES_HR = 1008000000;
  const vga_mode_t VGA_MODE_200_LINES_HR = VGA_MODE_640x480;
*/


//912Mhz 350 LINES PARADISE EGA
/*
  const String NAME_350_COLOR = "Mode 640x350x16 @60";
  const int MIN_CYCLES_HSYNC_350_COLOR  = 41697  ;
  const int CYCLES_HSYNC_HIRES_350_COLOR = 41000  ;
  const int CYCLES_FIRST_PIXEL_350_COLOR  = 1355 ; // 1346
  const int CYCLES_PIXEL_350_COLOR  = 56;
  const int CYCLES_PIXEL_350_COLOR_EXACT = 56099  ;
  const int CYCLES_HALF_PIXEL_350_COLOR_EXACT = 28049 ;
  const int CYCLES_HALF_PIXEL_350_COLOR = 28  ;
  const int CYCLES_PIXEL_350_COLOR_MAX_SKEW = 15  ; //15
  const int CYCLES_PIXEL_350_COLOR_PRESAMPLE = 10 ; //15
  const int MAX_CYCLES_VSYNC_350_COLOR = 15330430 ;
  const int MIN_CYCLES_VSYNC_350_COLOR = 15200000  ;
  const int BLANK_LINES_SCREEN_350_COLOR = 0  ;
  const int LINES_350_LINES = 352 ;
  const int MAX_LINES_350_LINES = 352;
  const int PIXELS_350_LINES = 640;
  const int SCANLINES_350_LINES = 1;
  const vga_mode_t VGA_MODE_350_LINES = VGA_MODE_640x480;
  const unsigned long CPU_FREQ_350_LINES = 912000000;
*/


//960Mhz 350 LINES PARADISE EGA
/*

  const String NAME_350_COLOR = "Mode 640x350x16 @60";
  const int MIN_CYCLES_HSYNC_350_COLOR  = 43892  ;
  const int CYCLES_HSYNC_HIRES_350_COLOR = 43700  ;
  const int CYCLES_FIRST_PIXEL_350_COLOR  = 1430 ;// 1425 ; // 1417
  const int CYCLES_PIXEL_350_COLOR  = 59;
  const int CYCLES_PIXEL_350_COLOR_EXACT = 59051  ;
  const int CYCLES_HALF_PIXEL_350_COLOR_EXACT = 29526 ;
  const int CYCLES_HALF_PIXEL_350_COLOR = 30  ;
  const int CYCLES_PIXEL_350_COLOR_MAX_SKEW = 15  ; //15
  const int CYCLES_PIXEL_350_COLOR_PRESAMPLE = 10 ; //15
  const int MAX_CYCLES_VSYNC_350_COLOR = 16202032 ;
  const int MIN_CYCLES_VSYNC_350_COLOR = 16000000  ;
  const int BLANK_LINES_SCREEN_350_COLOR = 0  ;
  const int LINES_350_LINES = 352 ;
  const int MAX_LINES_350_LINES = 352;
  const int PIXELS_350_LINES = 640;
  const int SCANLINES_350_LINES = 1;
  const vga_mode_t VGA_MODE_350_LINES = VGA_MODE_640x480;
  const unsigned long CPU_FREQ_350_LINES = 960000000;
*/


//972Mhz 350 LINES PARADISE EGA
/*
  const String NAME_350_COLOR = "Mode 640x350x16 @60";
  const int MIN_CYCLES_HSYNC_350_COLOR  = 43892  ;
  const int CYCLES_HSYNC_HIRES_350_COLOR = 43700  ;
  const int CYCLES_FIRST_PIXEL_350_COLOR  = 1450 ;// 1425 ; // 1435
  const int CYCLES_PIXEL_350_COLOR  = 60;
  const int CYCLES_PIXEL_350_COLOR_EXACT = 59790  ;
  const int CYCLES_HALF_PIXEL_350_COLOR_EXACT = 29895 ;
  const int CYCLES_HALF_PIXEL_350_COLOR = 30  ;
  const int CYCLES_PIXEL_350_COLOR_MAX_SKEW = 15  ; //15
  const int CYCLES_PIXEL_350_COLOR_PRESAMPLE = 10 ; //15
  const int MAX_CYCLES_VSYNC_350_COLOR = 16302032 ;
  const int MIN_CYCLES_VSYNC_350_COLOR = 16100000  ;
  const int BLANK_LINES_SCREEN_350_COLOR = 0  ;
  const int LINES_350_LINES = 352 ;
  const int MAX_LINES_350_LINES = 352;
  const int PIXELS_350_LINES = 640;
  const int SCANLINES_350_LINES = 1;
  const vga_mode_t VGA_MODE_350_LINES = VGA_MODE_640x480;
  const unsigned long CPU_FREQ_350_LINES = 984000000;
*/


//984Mhz 350 LINES PARADISE EGA
/**/
const String NAME_350_COLOR = "Mode 640x350x16 @60";
const int MIN_CYCLES_HSYNC_350_COLOR  = 44989  ;
const int CYCLES_HSYNC_HIRES_350_COLOR = 44600  ;
const int CYCLES_FIRST_PIXEL_350_COLOR  = 1541 ;
const int CYCLES_PIXEL_350_COLOR  = 61;
const int CYCLES_PIXEL_350_COLOR_EXACT = 60528  ;
const int CYCLES_HALF_PIXEL_350_COLOR_EXACT = 30264 ;
const int CYCLES_HALF_PIXEL_350_COLOR = 30  ;
const int CYCLES_PIXEL_350_COLOR_MAX_SKEW = 20  ; //15
const int CYCLES_PIXEL_350_COLOR_PRESAMPLE = 20 ; //15
const int MAX_CYCLES_VSYNC_350_COLOR = 16702032 ;
const int MIN_CYCLES_VSYNC_350_COLOR = 16300000  ;
const int BLANK_LINES_SCREEN_350_COLOR = 0  ;
const int LINES_350_LINES = 352 ;
const int PIXELS_350_LINES = 640;
const int SCANLINES_350_LINES = 1;
const vga_mode_t VGA_MODE_350_LINES = VGA_MODE_640x480;
const unsigned long CPU_FREQ_350_LINES = 984000000;



//1008Mhz 350 LINES PARADISE EGA
/*
  const String NAME_350_COLOR = "Mode 640x350x16 @60";
  const int MIN_CYCLES_HSYNC_350_COLOR  = 46087  ;
  const int CYCLES_HSYNC_HIRES_350_COLOR = 45800  ;
  const int CYCLES_FIRST_PIXEL_350_COLOR  = 1512  ; // 1512
  const int CYCLES_PIXEL_350_COLOR  = 62;
  const int CYCLES_PIXEL_350_COLOR_EXACT = 62004  ;
  const int CYCLES_HALF_PIXEL_350_COLOR_EXACT = 31002 ;
  const int CYCLES_HALF_PIXEL_350_COLOR = 31  ;
  const int CYCLES_PIXEL_350_COLOR_MAX_SKEW = 15  ; //15
  const int CYCLES_PIXEL_350_COLOR_PRESAMPLE = 10 ; //15
  const int MAX_CYCLES_VSYNC_350_COLOR = 17100000 ;
  const int MIN_CYCLES_VSYNC_350_COLOR = 16016016  ;
  const int BLANK_LINES_SCREEN_350_COLOR = 0  ;
  const int LINES_350_LINES = 352 ;
  const int MAX_LINES_350_LINES = 352;
  const int PIXELS_350_LINES = 640;
  const int SCANLINES_350_LINES = 1;
  const vga_mode_t VGA_MODE_350_LINES = VGA_MODE_640x480;
  const unsigned long CPU_FREQ_350_LINES = 1008000000;
*/


void setup() {

  pinMode(10, OUTPUT);
  pinMode(12, OUTPUT);
  pinMode(11, OUTPUT);

  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);

  pinMode(19, INPUT);
  pinMode(18, INPUT);
  pinMode(15, INPUT);

  pinMode(17, INPUT);
  pinMode(16, INPUT);
  pinMode(22, INPUT);
  pinMode(23, INPUT);
  pinMode(20, INPUT);
  pinMode(21, INPUT);

  pinMode(PIN_DEBUG, OUTPUT);
  pinMode(PIN_HR, INPUT_PULLUP);
  pinMode(PIN_RESET, INPUT_PULLUP);

  pinMode(PIN_PLUS, INPUT_PULLUP);
  pinMode(PIN_MINUS, INPUT_PULLUP);
  pinMode(PIN_STORE, INPUT_PULLUP);

  //Shit to initialize CPU cycle count readings
  ARM_DEMCR    |= ARM_DEMCR_TRCENA;
  ARM_DWT_CTRL |= ARM_DWT_CTRL_CYCCNTENA;
  ARM_DWT_CYCCNT = 0;


#ifdef DEBUG
  Serial.begin(57600);
  /*  while (!Serial) {
      ; // wait for serial port to connect.
    }*/

  Serial.println("[DEBUG] Starting...");
#endif

}

void yield () {} //Get rid of the hidden function that checks for serial input and such.
extern "C" uint32_t set_arm_clock(uint32_t frequency);

//Capture variables
unsigned long cyclesFirstPixelToUse = 0;
long cyclesFirstPixel = 0;
static unsigned cycleToPixel[75000];
static unsigned long pixelToCycle[1000];
static unsigned long pixelToCyclePresample[1000];
static unsigned long pixelToCycleLimit[1000];
static unsigned long pixelToCycleLimitLow[1000];
static unsigned pixelToFrameBuffer[360];
int osdEnabledCycles = 180;

//Buttons states
boolean stateButtonPlusPrevious = true;
boolean stateButtonMinusPrevious = true;
boolean stateButtonStorePrevious = true;
boolean stateButtonResetPrevious = true;
int pushedPlusCycles = 0;
int pushedMinusCycles = 0;
int pushedStoreCycles = 0;
int pushedResetCycles = 0;
boolean displayStoredMessage = false;

//Sync state
boolean sync  = false;



//Function that precalculates pixel positions in cycles, cycle positions in pixels, and framebuffer positions for lines, for more efficient capture.
FASTRUN void buildLookupTables(unsigned long cyclesFirstPixel , int fb_stride) {

  //Build lookup tables for cycle-to-pixel and pixel-to-cycle calculations
  for (int i = 0; i < PIXELS + 100; i++) {
    pixelToCycle[i]  =  ((i * (CYCLES_PIXEL_EXACT) ) / 1000) + cyclesFirstPixel + CYCLES_HALF_PIXEL ;
    pixelToCyclePresample[i]  =  pixelToCycle[i] - CYCLES_PIXEL_PRESAMPLE;
    pixelToCycleLimit[i] = pixelToCycle[i] + CYCLES_PIXEL_MAX_SKEW;
    pixelToCycleLimitLow[i] = pixelToCycle[i] - CYCLES_PIXEL_MAX_SKEW;
  }

  for (int j = 0; j < LINES + 10; j++) {
    pixelToFrameBuffer[j] = j * fb_stride * SCANLINES ;
  }

  for (int i = 0; i < 75000; i++) {
    cycleToPixel[i]  =  ((i - cyclesFirstPixel) * 1000 ) / CYCLES_PIXEL_EXACT;
  }

}





//Function that tries to guess where the first pixel is after HSYNC if AUTODISCOVERY is enabled
//The value is overriden if the user stores a setting for that mode
FASTRUN void sampleFirstPixelData(boolean horizontalPolarity) {
  noInterrupts();
  //Look for first non-zero pixel for 1000 Hsyncs
  //No need to check for ISR at this level because it is not initialized
  int hsyncs = 5000;
  unsigned long cyclesLastHsync;
  unsigned long cyclesCurrent;
  unsigned long cyclesRelativeHsync;
  boolean lastHsync;
  boolean levelHsync;
  while (hsyncs--) {
    lastHsync = !horizontalPolarity;;
    while (true) {
      levelHsync = digitalReadFast(PIN_HS);
      cyclesCurrent = ARM_DWT_CYCCNT;
      if (levelHsync == !horizontalPolarity   && levelHsync != lastHsync ) {
        cyclesCurrent -= 2;
        cyclesLastHsync = cyclesCurrent;
        //Hsync
        //Look for first pixel for some cycles
        //Take some space
        while (ARM_DWT_CYCCNT - cyclesLastHsync < 500) {}

        cyclesRelativeHsync = 0;
        while (cyclesRelativeHsync < 15000UL) {
          cyclesCurrent = ARM_DWT_CYCCNT;
          if (digitalReadFast(16)
              // || digitalReadFast(21)
              || digitalReadFast(23)) {

            cyclesRelativeHsync = cyclesCurrent - cyclesLastHsync - 5;
            if (!cyclesFirstPixel || cyclesRelativeHsync < cyclesFirstPixel) {
              cyclesFirstPixel = cyclesRelativeHsync;
            }
            break;
          }
          cyclesRelativeHsync = cyclesCurrent - cyclesLastHsync;
        }
        break;
      }
      lastHsync = levelHsync;
    }

  }

  interrupts();

#ifdef DEBUG
  Serial.print("First pixel detected at:");
  Serial.println(cyclesFirstPixel, DEC);
#endif


}




FASTRUN void loop() {

  boolean lastVsync  = false;
  boolean lastHsync  = false;
  unsigned long cyclesCurrent = ARM_DWT_CYCCNT;
  unsigned long cyclesLastVsync  = 0;
  unsigned long cyclesLastHsync  = 0;
  unsigned long cyclesLastPixel  = 0;
  unsigned long cyclesNextPixel = 0;
  unsigned long cyclesNextIsrStartExpected = 0;
  unsigned long cyclesNextIsrEndExpected = 0;
  unsigned long cyclesNextHsyncExpected = 0;
  long diffCyclesIsrStart = 0;
  long diffCyclesIsrEnd = 0;
  unsigned long difference = 0;
  unsigned long cyclesLastIsr = 0;
  unsigned long diffLastIsr = 0;
  int currentPixel = 0;
  int newPixel = 0;
  int pixelsSkipped = 0;
  int currentLine = 0;
  boolean testCycle = false;
  int inputState = STATE_WAIT_HSYNC;
  unsigned pixelCopy = 0;
  boolean levelVsync = false;
  boolean levelHsync = false;
  unsigned pixel;
  boolean skipHS = false;
  boolean firstHsync = true;
  int lastFiredIsr = 0;
  boolean skippedHsync = false;
  int countIsr = 0;
  int countIsr2 = 0;
  int countIsr3 = 0;
  int cyclesPreviousPixel = 0;
  int VerticalPolarity = 0;
  int HorizontalPolarity = 0;
  boolean hasFiredIsr = false;
  int expectedPixel = -1;
  int screens  = 0;
  unsigned cyclesRelativeHsync;
  boolean isr_fired = false;
  boolean vgaInitialized = false;
  unsigned long uptime = 0;
  long lastValidHsyncWidth = 0;
  long runningAverageHsyncWidth = 0;
  long runningAverageHsyncSamples = 0;
  boolean lastHsyncWasValid = false;



  //Forever
  while (1) {


#ifdef DEBUG
    Serial.print("Temperature:");
    Serial.println(tempmonGetTemp(), 1);
#endif

    while (!sync) {
      if (F_CPU_ACTUAL != 600000000UL) {
#ifdef DEBUG
        Serial.println("Changing CPU speed");
#endif
        set_arm_clock(600000000UL);
      }
      //Detect horizontal and vertical polarities
#ifdef DEBUG
      Serial.print("CPU speed:");
      Serial.println(F_CPU_ACTUAL, DEC);
#endif
      int samples = 10000000;
      int halfSamples = samples / 2;
      int countVsync = 0;
      int countHsync = 0;

      while (samples--) {
        countVsync += digitalReadFast(PIN_VS);
        countHsync += digitalReadFast(PIN_HS);
      }

      if (countVsync < halfSamples) {
        VerticalPolarity = POSITIVE;
#ifdef DEBUG
        Serial.println("Detected vertical polarity +");
#endif

      } else {
        VerticalPolarity = NEGATIVE;
#ifdef DEBUG
        Serial.println("Detected vertical polarity -");
#endif
      }

      if (countHsync < halfSamples) {
        HorizontalPolarity = POSITIVE;
#ifdef DEBUG
        Serial.println("Detected horizontal polarity +");
#endif
      } else {
        HorizontalPolarity = NEGATIVE;
#ifdef DEBUG
        Serial.println("Detected horizontal polarity -");
#endif
      }


      //Detect Horizontal frequency

      //Capture 1000 edges
      int prevHsync = HorizontalPolarity;
      int numEdges = 1000;
      unsigned long startCapture = 0;
      while (digitalReadFast(PIN_HS) == HorizontalPolarity) {

      }

      boolean currentHsync;
      prevHsync = !HorizontalPolarity;
      currentHsync = !HorizontalPolarity;
      while (numEdges--) {
        while (!(prevHsync == !HorizontalPolarity && currentHsync == HorizontalPolarity)) {
          prevHsync = currentHsync;
          currentHsync = digitalReadFast(PIN_HS);
        }
        //cycle
        if (!startCapture) {
          startCapture = ARM_DWT_CYCCNT;
        }

        prevHsync = currentHsync;
      }

      unsigned long  endCapture = ARM_DWT_CYCCNT;
      unsigned long  capturedCycles = endCapture - startCapture;
      int hfreq = F_CPU_ACTUAL / ( capturedCycles / 1000 );
#ifdef DEBUG
      Serial.print("Detected horizontal frequency:");
      Serial.println(hfreq, DEC);
#endif

      //Detect Vertical frequency

      //Capture 1000 edges
      int prevVsync = VerticalPolarity;
      numEdges = 30;
      startCapture = 0;
      while (digitalReadFast(PIN_VS) == VerticalPolarity) {

      }

      boolean currentVsync;
      prevVsync = !VerticalPolarity;
      currentVsync = !VerticalPolarity;
      while (numEdges--) {
        while (!(prevVsync == !VerticalPolarity && currentVsync == VerticalPolarity)) {
          prevVsync = currentVsync;
          currentVsync = digitalReadFast(PIN_VS);
        }
        //cycle
        if (!startCapture) {
          startCapture = ARM_DWT_CYCCNT;
        }

        prevVsync = currentVsync;
      }

      endCapture = ARM_DWT_CYCCNT;
      capturedCycles = endCapture - startCapture;
      int vfreq = F_CPU_ACTUAL / ( capturedCycles / 30 );
#ifdef DEBUG
      Serial.print("Detected vertical frequency:");
      Serial.println(vfreq, DEC);
#endif



      //Select input mode based on detected frequencies and polarities and initialize VGA_t4 and capture

      if (HorizontalPolarity && !VerticalPolarity && hfreq > 18400 && hfreq < 18500) {
        //Mode is 350 lines monochrome
        //NOT IMPLEMENTED as there are no 720 pixel modes in VGA_T4
        //currentMode = MODE_350_MONO;
#ifdef DEBUG
        Serial.println("Mode is 350 lines monochrome, not implemented");
#endif


      } else if (HorizontalPolarity && VerticalPolarity && hfreq > 15700 && hfreq < 15800) {

        //Check if HR is enabled reading th switch
        if (digitalReadFast(PIN_HR)) {
          // enabled
#ifdef DEBUG
          Serial.println("Mode is 200 lines color High Resolution");
#endif
          //HIGH RES MODE
          //Mode is 200 lines hr
          currentMode = MODE_200_COLOR_HR;

          //Setting magic numbers
          NAME = NAME_200_COLOR_HR;
          MIN_CYCLES_HSYNC  = MIN_CYCLES_HSYNC_200_COLOR_HR  ;
          CYCLES_HSYNC_HIRES = CYCLES_HSYNC_HIRES_200_COLOR_HR  ;
          CYCLES_FIRST_PIXEL  = CYCLES_FIRST_PIXEL_200_COLOR_HR;
          CYCLES_PIXEL  = CYCLES_PIXEL_200_COLOR_HR  ;
          CYCLES_PIXEL_EXACT = CYCLES_PIXEL_200_COLOR_HR_EXACT  ;
          CYCLES_HALF_PIXEL_EXACT = CYCLES_HALF_PIXEL_200_COLOR_HR_EXACT ;
          CYCLES_HALF_PIXEL = CYCLES_HALF_PIXEL_200_COLOR_HR  ;
          CYCLES_PIXEL_MAX_SKEW = CYCLES_PIXEL_200_COLOR_HR_MAX_SKEW  ;
          CYCLES_PIXEL_PRESAMPLE = CYCLES_PIXEL_200_COLOR_HR_PRESAMPLE;
          MAX_CYCLES_VSYNC = MAX_CYCLES_VSYNC_200_COLOR_HR ;
          MIN_CYCLES_VSYNC = MIN_CYCLES_VSYNC_200_COLOR_HR  ;
          BLANK_LINES_SCREEN = BLANK_LINES_SCREEN_200_COLOR_HR ;
          LINES = LINES_200_LINES_HR;
          PIXELS = PIXELS_200_LINES_HR;
          SCANLINES = SCANLINES_200_LINES_HR;
          CPU_FREQ  = CPU_FREQ_200_LINES_HR;
          VGA_MODE = VGA_MODE_200_LINES_HR;

          //Init screen
          if (!vgaInitialized || vgaMode != VGA_MODE_200_LINES_HR) {
            //set_arm_clock(600000000);
            vga_error_t err = vga.begin(VGA_MODE_200_LINES_HR, &isr_fired, CPU_FREQ_200_LINES_HR);
            if (err != 0)
            {
#ifdef DEBUG
              Serial.println("fatal error");
#endif
              while (1);
            }

            set_arm_clock(CPU_FREQ_200_LINES_HR);

#ifdef DEBUG
            Serial.print("CPU speed:");
            Serial.println(F_CPU_ACTUAL, DEC);
#endif

            vgaInitialized = true;
            vgaMode = VGA_MODE_200_LINES_HR;
          }

#ifdef AUTODISCOVERY
          //Find location of the first pixel data if AUTODISCOVERY is enabled
          sampleFirstPixelData(HorizontalPolarity);
#endif

          //load previous value from EEPROM
          unsigned storedFirstPixel = EEPROMReadInt(EEPROM_CYCLES_FIRST_PIXEL_MODE_200_COLOR_HR);
          unsigned userFirstPixel = EEPROMReadInt(EEPROM_CYCLES_FIRST_PIXEL_MODE_200_COLOR_HR_USER_DEFINED);

#ifdef DEBUG
          Serial.print("Stored first pixel:");
          Serial.println(storedFirstPixel, DEC);
          Serial.print("User first pixel:");
          Serial.println(userFirstPixel, DEC);
#endif

          if (userFirstPixel > 0 && userFirstPixel < 6000) {
            cyclesFirstPixelToUse = userFirstPixel;
          } else {

            //Not user defined value available
            if (cyclesFirstPixel &&  cyclesFirstPixel > 6000 &&  ((cyclesFirstPixel < storedFirstPixel) ||  ((storedFirstPixel < 6000) && (cyclesFirstPixel > 6000)))) {
              //Update stored value
              Serial.println("Updating EEPROM value");
              EEPROMWriteInt(EEPROM_CYCLES_FIRST_PIXEL_MODE_200_COLOR_HR, cyclesFirstPixel);
            }

            else if (storedFirstPixel < 20000 && storedFirstPixel > 6000) {
              cyclesFirstPixel = storedFirstPixel;
            }

            cyclesFirstPixelToUse = CYCLES_FIRST_PIXEL_200_COLOR_HR;

            if (cyclesFirstPixel) {
              cyclesFirstPixelToUse = cyclesFirstPixel;
            }

          }


#ifdef DEBUG
          Serial.print("Using first pixel value:");
          Serial.println(cyclesFirstPixelToUse, DEC);
#endif

          //Build lookup tables for cycle-to-pixel and pixel-to-cycle calculations
          int fb_stride = vga.get_fb_stride();

          buildLookupTables(cyclesFirstPixelToUse,
                            fb_stride
                           );


        } else {

          //LOW RES MODE
          //Mode is 200 lines hr
          currentMode = MODE_200_COLOR;
#ifdef DEBUG
          Serial.println("Mode is 200 lines color Low Resolution");
#endif

          //Setting magic numbers
          NAME = NAME_200_COLOR;
          MIN_CYCLES_HSYNC  = MIN_CYCLES_HSYNC_200_COLOR  ;
          CYCLES_HSYNC_HIRES = CYCLES_HSYNC_HIRES_200_COLOR  ;
          CYCLES_FIRST_PIXEL  = CYCLES_FIRST_PIXEL_200_COLOR;
          CYCLES_PIXEL  = CYCLES_PIXEL_200_COLOR  ;
          CYCLES_PIXEL_EXACT = CYCLES_PIXEL_200_COLOR_EXACT  ;
          CYCLES_HALF_PIXEL_EXACT = CYCLES_HALF_PIXEL_200_COLOR_EXACT ;
          CYCLES_HALF_PIXEL = CYCLES_HALF_PIXEL_200_COLOR  ;
          CYCLES_PIXEL_MAX_SKEW = CYCLES_PIXEL_200_COLOR_MAX_SKEW  ;
          CYCLES_PIXEL_PRESAMPLE = CYCLES_PIXEL_200_COLOR_PRESAMPLE;
          MAX_CYCLES_VSYNC = MAX_CYCLES_VSYNC_200_COLOR ;
          MIN_CYCLES_VSYNC = MIN_CYCLES_VSYNC_200_COLOR  ;
          BLANK_LINES_SCREEN = BLANK_LINES_SCREEN_200_COLOR ;
          LINES = LINES_200_LINES;
          PIXELS = PIXELS_200_LINES;
          SCANLINES = SCANLINES_200_LINES;
          CPU_FREQ  = CPU_FREQ_200_LINES;
          VGA_MODE = VGA_MODE_200_LINES;

          //Init screen
          if (!vgaInitialized || vgaMode != VGA_MODE_200_LINES) {
            //set_arm_clock(600000000);
            vga_error_t err = vga.begin(VGA_MODE_200_LINES, &isr_fired, CPU_FREQ_200_LINES);
            if (err != 0)
            {
#ifdef DEBUG
              Serial.println("fatal error");
#endif
              while (1);
            }

            set_arm_clock(CPU_FREQ_200_LINES);
#ifdef DEBUG
            Serial.print("CPU speed:");
            Serial.println(F_CPU_ACTUAL, DEC);
#endif
            vgaInitialized = true;
            vgaMode = VGA_MODE_200_LINES;
          }

#ifdef AUTODISCOVERY
          //Find location of the first pixel data if AUTODISCOVERY is enabled
          sampleFirstPixelData(HorizontalPolarity);
#endif

          //load previous value from EEPROM
          unsigned storedFirstPixel = EEPROMReadInt(EEPROM_CYCLES_FIRST_PIXEL_MODE_200_COLOR);
          unsigned userFirstPixel = EEPROMReadInt(EEPROM_CYCLES_FIRST_PIXEL_MODE_200_COLOR_USER_DEFINED);


#ifdef DEBUG
          Serial.print("Stored first pixel:");
          Serial.println(storedFirstPixel, DEC);
#endif


          if (userFirstPixel > 0 && userFirstPixel < 4000) {
            cyclesFirstPixelToUse = userFirstPixel;
          } else {

            //Not user defined value available

            if (cyclesFirstPixel &&  cyclesFirstPixel > 4000 && ((cyclesFirstPixel < storedFirstPixel) ||  ((storedFirstPixel < 4000) && (cyclesFirstPixel > 4000)))) {
              //Update stored value
              Serial.println("Updating EEPROM value");
              EEPROMWriteInt(EEPROM_CYCLES_FIRST_PIXEL_MODE_200_COLOR, cyclesFirstPixel);
            }

            else if (storedFirstPixel < 20000 && storedFirstPixel > 4000) {
              cyclesFirstPixel = storedFirstPixel;
            }

            cyclesFirstPixelToUse = CYCLES_FIRST_PIXEL_200_COLOR;

            if (cyclesFirstPixel) {
              cyclesFirstPixelToUse = cyclesFirstPixel;
            }
          }

#ifdef DEBUG
          Serial.print("Using first pixel value:");
          Serial.println(cyclesFirstPixelToUse, DEC);
#endif

          //Build lookup table for cycle-to-pixel and pixel-to-cycle calculations
          int fb_stride = vga.get_fb_stride();
          buildLookupTables(cyclesFirstPixelToUse,
                            fb_stride
                           );

        }


      } else if (HorizontalPolarity && !VerticalPolarity && hfreq > 21600 && hfreq < 21900) {
        //Mode is 350 lines color
#ifdef DEBUG
        Serial.println("Mode is 350 lines color");
#endif
        currentMode = MODE_350_COLOR;

        //Setting magic numbers
        NAME = NAME_350_COLOR;
        MIN_CYCLES_HSYNC  = MIN_CYCLES_HSYNC_350_COLOR  ;
        CYCLES_HSYNC_HIRES = CYCLES_HSYNC_HIRES_350_COLOR  ;
        CYCLES_FIRST_PIXEL  = CYCLES_FIRST_PIXEL_350_COLOR;
        CYCLES_PIXEL  = CYCLES_PIXEL_350_COLOR  ;
        CYCLES_PIXEL_EXACT = CYCLES_PIXEL_350_COLOR_EXACT  ;
        CYCLES_HALF_PIXEL_EXACT = CYCLES_HALF_PIXEL_350_COLOR_EXACT ;
        CYCLES_HALF_PIXEL = CYCLES_HALF_PIXEL_350_COLOR  ;
        CYCLES_PIXEL_MAX_SKEW = CYCLES_PIXEL_350_COLOR_MAX_SKEW  ;
        CYCLES_PIXEL_PRESAMPLE = CYCLES_PIXEL_350_COLOR_PRESAMPLE;
        MAX_CYCLES_VSYNC = MAX_CYCLES_VSYNC_350_COLOR ;
        MIN_CYCLES_VSYNC = MIN_CYCLES_VSYNC_350_COLOR  ;
        BLANK_LINES_SCREEN = BLANK_LINES_SCREEN_350_COLOR ;
        LINES = LINES_350_LINES;
        PIXELS = PIXELS_350_LINES;
        SCANLINES = SCANLINES_350_LINES;
        CPU_FREQ  = CPU_FREQ_350_LINES;
        VGA_MODE = VGA_MODE_350_LINES;

        //Init screen
        if (!vgaInitialized || vgaMode != VGA_MODE_350_LINES) {
          set_arm_clock(600000000);
          //Init screen
          vga_error_t err = vga.begin(VGA_MODE_350_LINES, &isr_fired, CPU_FREQ_350_LINES);
          if (err != 0)
          {
#ifdef DEBUG
            Serial.println("fatal error");
#endif
            while (1);
          }

#ifdef DEBUG
          Serial.println("VGA initialized");
#endif
          set_arm_clock(CPU_FREQ_350_LINES);
#ifdef DEBUG
          Serial.print("CPU speed:");
          Serial.println(F_CPU_ACTUAL, DEC);
#endif

          vgaInitialized = true;
          vgaMode = VGA_MODE_350_LINES;

        }

#ifdef AUTODISCOVERY
        //Find location of the first pixel data if AUTODISCOVERY is enabled
        sampleFirstPixelData(HorizontalPolarity);
#endif

        //load previous value from EEPROM
        unsigned storedFirstPixel = EEPROMReadInt(EEPROM_CYCLES_FIRST_PIXEL_MODE_350_COLOR);
        unsigned userFirstPixel = EEPROMReadInt(EEPROM_CYCLES_FIRST_PIXEL_MODE_350_COLOR_USER_DEFINED);


#ifdef DEBUG
        Serial.print("Stored first pixel:");
        Serial.println(storedFirstPixel, DEC);
#endif


        if (userFirstPixel > 0 && userFirstPixel < 4000) {
          cyclesFirstPixelToUse = userFirstPixel;
        } else {

          //Not user defined value available
          if (cyclesFirstPixel &&  cyclesFirstPixel > 1000 && ((cyclesFirstPixel < storedFirstPixel) ||  ((storedFirstPixel < 1000) && (cyclesFirstPixel > 1000)))) {
            //Update stored value
            Serial.println("Updating EEPROM value");
            EEPROMWriteInt(EEPROM_CYCLES_FIRST_PIXEL_MODE_350_COLOR, cyclesFirstPixel);
          }

          else if (storedFirstPixel < 20000 && storedFirstPixel > 1000) {
            cyclesFirstPixel = storedFirstPixel;
          }

          cyclesFirstPixelToUse = CYCLES_FIRST_PIXEL_350_COLOR;

          if (cyclesFirstPixel) {
            cyclesFirstPixelToUse = cyclesFirstPixel;
          }
        }

#ifdef DEBUG
        Serial.print("Using first pixel value:");
        Serial.println(cyclesFirstPixelToUse, DEC);
#endif

        //Build lookup table for cycle-to-pixel and pixel-to-cycle calculations
        int fb_stride = vga.get_fb_stride();
        buildLookupTables(cyclesFirstPixelToUse,
                          fb_stride
                         );
      }


      if (currentMode) {
#ifdef DEBUG
        Serial.println("Trying to sync to signal...");
#endif
        lastVsync = VerticalPolarity;
      }

      //sync to first vsync
      if (currentMode == MODE_350_COLOR) {
        set_arm_clock(CPU_FREQ_350_LINES);
      } else if (currentMode == MODE_200_COLOR) {
        set_arm_clock(CPU_FREQ_200_LINES);
      } else if (currentMode == MODE_200_COLOR_HR) {
        set_arm_clock(CPU_FREQ_200_LINES_HR);
      }
#ifdef DEBUG
      Serial.print("CPU speed:");
      Serial.println(F_CPU_ACTUAL, DEC);
#endif


      //Sync to VSYNC
      while (currentMode && !sync) {
        levelVsync = digitalReadFast(PIN_VS);
        cyclesCurrent = ARM_DWT_CYCCNT;
        if (levelVsync == VerticalPolarity && (levelVsync != lastVsync) ) {
          //transition
#ifdef DEBUG
          Serial.println((cyclesCurrent - cyclesLastVsync) , DEC);
#endif
          if ((cyclesCurrent - cyclesLastVsync) <= MAX_CYCLES_VSYNC && (cyclesCurrent - cyclesLastVsync) >= MIN_CYCLES_VSYNC ) {
            //Sync
            sync = true;
          }
          cyclesLastVsync = cyclesCurrent;
        }
        lastVsync = levelVsync;
      }
    }
#ifdef DEBUG
    Serial.println("Synced");
#endif

    inputState = STATE_WAIT_HSYNC;
    cyclesLastHsync = 0;
    osdEnabledCycles = 300;

    pixelCopy = 0;
    int fb_stride = vga.get_fb_stride();

    vga_pixel * framebuffer = vga.get_frame_buffer();
    vga.clear(VGA_RGB(0x00, 0x00, 0x00));
#ifdef DEBUG
    Serial.print("CPU speed:");
    Serial.println(F_CPU_ACTUAL, DEC);
#endif
    while (sync) {



      //All the code for the different modes is a copy-paste, but it doesn't work fast enough if not using constants in some critical sections so I need to keep them separated


      //
      //
      // MODE 200 LINES COLOR LOW RES
      //
      //


      if (currentMode == MODE_200_COLOR) {
#ifdef DEBUG
        Serial.println("Entering 200 Color Low Resolution Mode");
#endif
      }
      while (currentMode == MODE_200_COLOR) {

        //INPUT SECTION
        cyclesCurrent = ARM_DWT_CYCCNT;
        //hsync can be 18.43 kHz, 15.7 kHz, 21.85 kHz
        if (inputState == STATE_WAIT_HSYNC) {

          int countValidHS = 0;

          while (inputState == STATE_WAIT_HSYNC) {

            //Get number of time ISR has fired, to detect if it fires up during sampling
            countIsr = vga.getFiredIsr();
            levelHsync = digitalReadFast(PIN_HS);
            cyclesCurrent = ARM_DWT_CYCCNT;
            countIsr2 = vga.getFiredIsr();


            if (levelHsync == !HorizontalPolarity  && levelHsync != lastHsync ) {
              //HSYNC DETECTED

              if (!firstHsync && (countIsr != countIsr2 || countIsr != countIsr3)) {
                //Isr fired during sampling, so not reliable and we skip the line
                skipHS = true;

              }
              cyclesCurrent += 5; //ADJUST TO ACCOUNT FOR LOOP CYCLES

              currentLine++;
              if (currentLine >= 0) {
                inputState = STATE_WAIT_FIRST_PIXEL;
                //
              }

              if (skipHS) {

                countValidHS = 0;
                //digitalWriteFast(6, !digitalReadFast(6));
                //skip line
                skipHS = false;
                cyclesLastHsync = cyclesLastHsync + MIN_CYCLES_HSYNC_200_COLOR;
                inputState = STATE_WAIT_HSYNC;

                if (currentLine == LINES_200_LINES)   {
                  //End of screen
                  inputState = STATE_WAIT_VSYNC;
                  lastVsync = digitalReadFast(PIN_VS);

                }
                //sleep 2/3 of HS
                unsigned long cycles = cyclesCurrent;
                while (cyclesCurrent - cycles < ((MIN_CYCLES_HSYNC_200_COLOR * 2) / 3) ) {
                  cyclesCurrent = ARM_DWT_CYCCNT;
                }
              }
              else {
                //Not skipped
                if (countValidHS == 1 && ((cyclesCurrent - cyclesLastHsync < MIN_CYCLES_HSYNC_200_COLOR * 0.90) || (cyclesCurrent - cyclesLastHsync > MIN_CYCLES_HSYNC_200_COLOR * 1.10) )) {
                  currentMode = 0;
                  sync = false;
#ifdef DEBUG
                  Serial.println("Out of sync HS length");
#endif
                  break;
                }

                countValidHS++;

                cyclesLastHsync = cyclesCurrent;
                //digitalWriteFast(6, !digitalReadFast(6));
                if (currentLine < 0) {
                  //sleep 2/3 of HS
                  unsigned long cycles = cyclesCurrent;
                  while (cyclesCurrent - cycles < ((MIN_CYCLES_HSYNC_200_COLOR * 2) / 3) ) {
                    cyclesCurrent = ARM_DWT_CYCCNT;
                  }
                }
              }

              firstHsync = false;

            } //end hsync detected

            countIsr3 = countIsr;
            lastHsync = levelHsync;
          }
        }

        else if (inputState == STATE_WAIT_FIRST_PIXEL) {

          /*while (cyclesCurrent < (cyclesLastHsync + cyclesFirstPixelToUse - 500)) {
            cyclesCurrent = ARM_DWT_CYCCNT;

            }*/

          inputState = STATE_READING_PIXELS;
          expectedPixel = 0;
          cyclesNextPixel = pixelToCyclePresample[expectedPixel];
        }

        else if (inputState == STATE_READING_PIXELS) {

          while ((inputState == STATE_READING_PIXELS)) {
            isr_fired = false;
            while ((ARM_DWT_CYCCNT - cyclesLastHsync) < (cyclesNextPixel)) {}
            pixel = PINS_COLOR;
            cyclesCurrent = ARM_DWT_CYCCNT;
            cyclesRelativeHsync = cyclesCurrent - cyclesLastHsync;



            //calculate pixel position and deviation from ideal sampling point
            currentPixel = cycleToPixel[cyclesRelativeHsync];

            //Discard if unexpected pixel
            if (currentPixel != expectedPixel) {
              isr_fired = true;
            }

            //Discard if too late
            else if ((cyclesRelativeHsync) > pixelToCycleLimit[currentPixel] ) {
              isr_fired = true;
              //digitalWriteFast(PIN_DEBUG, !digitalReadFast(PIN_DEBUG));
            }

            expectedPixel = currentPixel + 1;

            if (!isr_fired && currentPixel <= PIXELS_200_LINES) {
              //Write pixel
              framebuffer[pixelToFrameBuffer[currentLine] + currentPixel]  = pixelTranslation[CGA][pixel];
              //Uncomment to adjust presampling
              /*
                Serial.println(cyclesRelativeHsync, DEC);
                Serial.println(currentPixel, DEC);
                Serial.println(currentLine, DEC);
                Serial.println(cyclesRelativeHsync - pixelToCyclePresample[currentPixel], DEC);
                Serial.println(cyclesRelativeHsync - pixelToCycle[currentPixel], DEC);
                Serial.println("");
              */
              //framebuffer[pixelToFrameBuffer[currentLine] +fb_stride + currentPixel]  = pixelTranslation[CGA][pixel];
            }

            if (currentPixel > PIXELS_200_LINES) {
              //End of line
              inputState = STATE_WAIT_HSYNC;
              if (currentLine >= LINES_200_LINES)   {
                //End of screen
                inputState = STATE_WAIT_VSYNC;
                lastVsync = digitalReadFast(PIN_VS);

              }
            }
            cyclesNextPixel = pixelToCyclePresample[expectedPixel];
          }
        }

        else if (inputState == STATE_WAIT_VSYNC)  {

          while ( (cyclesCurrent - cyclesLastVsync) < MIN_CYCLES_VSYNC_200_COLOR )
          {
            cyclesCurrent = ARM_DWT_CYCCNT;
          }
          //Time to sample VS
          levelVsync = digitalReadFast(PIN_VS);

          if (levelVsync && levelVsync != lastVsync ) {
            cyclesLastVsync = cyclesCurrent;
            cyclesLastHsync = cyclesCurrent - CYCLES_HSYNC_HIRES_200_COLOR;
            inputState = STATE_WAIT_HSYNC;
            firstHsync = true;
            currentLine = -BLANK_LINES_SCREEN_200_COLOR;
            testCycle = !testCycle;
            //digitalWriteFast(5, testCycle);
#ifdef DEBUG
            uptime++;
            if ( uptime % 60 == 0)
            {
              Serial.print("Temperature:");
              Serial.print(tempmonGetTemp(), 1);
              Serial.print("  Uptime:");
              Serial.println(uptime / 60);
            }

#endif

            //We have some free clock cycles to process OSD
            osd(NAME, vga);
          }
          lastVsync = levelVsync;
        }

      }






      //
      //
      // MODE 200 LINES COLOR HIGH RES
      //
      //



      if (currentMode == MODE_200_COLOR_HR) {
#ifdef DEBUG
        Serial.println("Entering 200 Color High Resolution Mode");
#endif
      }
      while (currentMode == MODE_200_COLOR_HR) {

        //INPUT SECTION
        cyclesCurrent = ARM_DWT_CYCCNT;
        //hsync can be 18.43 kHz, 15.7 kHz, 21.85 kHz
        if (inputState == STATE_WAIT_HSYNC) {

          int countValidHS = 0;

          while (inputState == STATE_WAIT_HSYNC) {
            //Get number of time ISR has fired, to detect if it fires up during sampling
            countIsr = vga.getFiredIsr();
            levelHsync = digitalReadFast(PIN_HS);
            cyclesCurrent = ARM_DWT_CYCCNT;
            countIsr2 = vga.getFiredIsr();


            if (levelHsync == !HorizontalPolarity  && levelHsync != lastHsync ) {
              //HSYNC DETECTED

              if (!firstHsync && (countIsr != countIsr2 || countIsr != countIsr3)) {
                //Isr fired during sampling, so not reliable and we skip the line
                skipHS = true;
              }

              cyclesCurrent += 5; //ADJUST TO ACCOUNT FOR LOOP CYCLES
              currentLine++;
              if (currentLine >= 0) {
                inputState = STATE_WAIT_FIRST_PIXEL;
              }

              if (skipHS) {

                countValidHS = 0;
                //digitalWriteFast(6, !digitalReadFast(6));
                //skip line
                skipHS = false;
                cyclesLastHsync = cyclesLastHsync + MIN_CYCLES_HSYNC_200_COLOR_HR;
                inputState = STATE_WAIT_HSYNC;

                if (currentLine == LINES_200_LINES_HR)   {
                  //End of screen
                  inputState = STATE_WAIT_VSYNC;
                  lastVsync = digitalReadFast(PIN_VS);

                }
                //sleep 2/3 of HS
                unsigned long cycles = cyclesCurrent;
                while (cyclesCurrent - cycles < ((MIN_CYCLES_HSYNC_200_COLOR_HR * 2) / 3) ) {
                  cyclesCurrent = ARM_DWT_CYCCNT;
                }
              }
              else {
                //Not skipped
                if (countValidHS == 1 && ((cyclesCurrent - cyclesLastHsync < MIN_CYCLES_HSYNC_200_COLOR_HR * 0.90) || (cyclesCurrent - cyclesLastHsync > MIN_CYCLES_HSYNC_200_COLOR_HR * 1.10) )) {
                  currentMode = 0;
                  sync = false;
#ifdef DEBUG
                  Serial.println("Out of sync HS length");
#endif
                  break;
                }
                countValidHS++;
                cyclesLastHsync = cyclesCurrent;
                //digitalWriteFast(6, !digitalReadFast(6));
                if (currentLine < 0) {
                  //sleep 2/3 of HS
                  unsigned long cycles = cyclesCurrent;
                  while (cyclesCurrent - cycles < ((MIN_CYCLES_HSYNC_200_COLOR_HR * 2) / 3) ) {
                    cyclesCurrent = ARM_DWT_CYCCNT;
                  }
                }
              }
              firstHsync = false;
            } //end hsync detected
            countIsr3 = countIsr;
            lastHsync = levelHsync;
          }
        }

        else if (inputState == STATE_WAIT_FIRST_PIXEL) {
          while (cyclesCurrent < (cyclesLastHsync + cyclesFirstPixelToUse - 500 )) {
            cyclesCurrent = ARM_DWT_CYCCNT;
          }
          inputState = STATE_READING_PIXELS;
          expectedPixel = 0;
          cyclesNextPixel = pixelToCyclePresample[expectedPixel];
        }

        else if (inputState == STATE_READING_PIXELS) {

          while ((inputState == STATE_READING_PIXELS)) {
            isr_fired = false;
            while ((ARM_DWT_CYCCNT - cyclesLastHsync) < (cyclesNextPixel)) {}
            pixel = PINS_COLOR;
            cyclesCurrent = ARM_DWT_CYCCNT;
            cyclesRelativeHsync = cyclesCurrent - cyclesLastHsync;

            //calculate pixel position and deviation from ideal sampling point
            currentPixel = cycleToPixel[cyclesRelativeHsync];

            //Discard if unexpected pixel
            if (currentPixel != expectedPixel) {
              isr_fired = true;
            }

            //Discard if too late
            else if ((cyclesRelativeHsync) > pixelToCycleLimit[currentPixel] ) {
              isr_fired = true;
            }

            expectedPixel = currentPixel + 1;

            if (!isr_fired && currentPixel <= PIXELS_200_LINES_HR) {
              //Write pixel
              framebuffer[pixelToFrameBuffer[currentLine] + currentPixel]  = pixelTranslation[CGA][pixel];
              //Uncomment to adjust presampling
              /*
                Serial.println(cyclesRelativeHsync, DEC);
                Serial.println(currentPixel, DEC);
                Serial.println(currentLine, DEC);
                Serial.println(cyclesRelativeHsync - pixelToCyclePresample[currentPixel], DEC);
                Serial.println(cyclesRelativeHsync - pixelToCycle[currentPixel], DEC);
                Serial.println("");
              */
            }

            if (currentPixel >= PIXELS_200_LINES_HR) {
              //End of line
              inputState = STATE_WAIT_HSYNC;
              if (currentLine == LINES_200_LINES_HR)   {
                //End of screen
                inputState = STATE_WAIT_VSYNC;
                lastVsync = digitalReadFast(PIN_VS);
              }
            }
            cyclesNextPixel = pixelToCyclePresample[expectedPixel];
          }
        }

        else if (inputState == STATE_WAIT_VSYNC)  {

          while ( (cyclesCurrent - cyclesLastVsync) < MIN_CYCLES_VSYNC_200_COLOR_HR )
          {
            cyclesCurrent = ARM_DWT_CYCCNT;
          }
          //Time to sample VS
          levelVsync = digitalReadFast(PIN_VS);

          if (levelVsync && levelVsync != lastVsync ) {
            cyclesLastVsync = cyclesCurrent;
            cyclesLastHsync = cyclesCurrent - CYCLES_HSYNC_HIRES_200_COLOR_HR;
            inputState = STATE_WAIT_HSYNC;
            firstHsync = true;
            currentLine = -BLANK_LINES_SCREEN_200_COLOR_HR;
            testCycle = !testCycle;
            //digitalWriteFast(5, testCycle);
#ifdef DEBUG
            uptime++;
            if ( uptime % 60 == 0)
            {
              Serial.print("Temperature:");
              Serial.print(tempmonGetTemp(), 1);
              Serial.print("  Uptime:");
              Serial.println(uptime / 60);
            }

#endif
            //We have some free clock cycles to process OSD
            osd(NAME, vga);
          }
          lastVsync = levelVsync;
        }


      }







      //
      //
      // MODE 350 LINES COLOR HIGH RES
      //
      //


      //MODE 350 COLOR

      if (currentMode == MODE_350_COLOR) {
#ifdef DEBUG
        Serial.println("Entering 350 Color Mode");
#endif
        //currentLine = -BLANK_LINES_SCREEN_350_COLOR;
      }

      while (currentMode == MODE_350_COLOR) {

        //INPUT SECTION
        cyclesCurrent = ARM_DWT_CYCCNT;
        //hsync can be 18.43 kHz, 15.7 kHz, 21.85 kHz
        if (inputState == STATE_WAIT_HSYNC) {

          int countValidHS = 0;

          while (inputState == STATE_WAIT_HSYNC) {

            //Get number of time ISR has fired, to detect if it fires up during sampling
            countIsr = vga.getFiredIsr();
            levelHsync = digitalReadFast(PIN_HS);
            cyclesCurrent = ARM_DWT_CYCCNT;
            countIsr2 = vga.getFiredIsr();

            if (levelHsync == !HorizontalPolarity  && levelHsync != lastHsync ) {
              //HSYNC DETECTED
              cyclesCurrent += 5; //ADJUST TO ACCOUNT FOR LOOP CYCLES

              if ((countIsr != countIsr2 || countIsr != countIsr3)) {
                //We are into a NON valid HSYNC
                //Isr fired during sampling, so not reliable and we skip the line
                //skipHS = true;

                //Hack to try to use this lines anyway as there is a strange pattern emerging at approx 1/3 of the screen width
                //where the pixels update less, somehow this is synchonizing with the ISR just at that moment in some lines
                //so when an HSYNC sample fails we use a running average of the HSYNC width over the last good HSYNC instead of discarding
                //This is preferred over having another magic number for this mode
                if (lastValidHsyncWidth != 0 && !firstHsync ) {
                  cyclesCurrent = cyclesLastHsync + runningAverageHsyncWidth ;
                } else {
                  //We still don't have a calculated running average
                  skipHS = true;
                }
                lastHsyncWasValid = false;
              }

              else {
                if (lastHsyncWasValid  && ((cyclesCurrent - cyclesLastHsync) < 50000)) {
                  //We are into a valid HSYNC

                  lastValidHsyncWidth = (cyclesCurrent - cyclesLastHsync);

                  if (runningAverageHsyncWidth == 0) {
                    runningAverageHsyncWidth = (cyclesCurrent - cyclesLastHsync);
                    runningAverageHsyncSamples = 1;
                  } else {
                    //Update hsync width running average
                    runningAverageHsyncSamples ++;
                    runningAverageHsyncWidth = runningAverageHsyncWidth + ((lastValidHsyncWidth - runningAverageHsyncWidth) / runningAverageHsyncSamples);

                  }
                }
                lastHsyncWasValid = true;
              }

              //Continue processing the HSYNC
              currentLine++;
              if (currentLine >= 0) {
                inputState = STATE_WAIT_FIRST_PIXEL;
                //
              }
              if (skipHS) {
                countValidHS = 0;
                //digitalWriteFast(6, !digitalReadFast(6));
                //skip line
                skipHS = false;
                cyclesLastHsync = cyclesLastHsync + MIN_CYCLES_HSYNC_350_COLOR;
                inputState = STATE_WAIT_HSYNC;

                if (currentLine == LINES_350_LINES)   {
                  //End of screen
                  inputState = STATE_WAIT_VSYNC;
                  lastVsync = digitalReadFast(PIN_VS);

                }
                //sleep 2/3 of HS
                unsigned long cycles = cyclesCurrent;
                while (cyclesCurrent - cycles < ((MIN_CYCLES_HSYNC_350_COLOR * 2) / 3) ) {
                  cyclesCurrent = ARM_DWT_CYCCNT;
                }
              }
              else {
                //Not skipped
                cyclesLastHsync = cyclesCurrent;

                //digitalWriteFast(6, !digitalReadFast(6));
                if (currentLine < 0) {
                  //sleep 2/3 of HS
                  unsigned long cycles = cyclesCurrent;
                  while (cyclesCurrent - cycles < ((MIN_CYCLES_HSYNC_350_COLOR * 2) / 3) ) {
                    cyclesCurrent = ARM_DWT_CYCCNT;
                  }
                }
              }
              firstHsync = false;
            } //end hsync detected
            countIsr3 = countIsr;
            lastHsync = levelHsync;
          }
        }

        else if (inputState == STATE_WAIT_FIRST_PIXEL) {

          while (cyclesCurrent < (cyclesLastHsync + cyclesFirstPixelToUse - 500)) {
            cyclesCurrent = ARM_DWT_CYCCNT;
          }

          inputState = STATE_READING_PIXELS;
          expectedPixel = 0;
          cyclesNextPixel = pixelToCyclePresample[expectedPixel];

          while (cyclesCurrent < cyclesNextPixel) {
            cyclesCurrent = ARM_DWT_CYCCNT;
          }

        }

        else if (inputState == STATE_READING_PIXELS) {

          while ((inputState == STATE_READING_PIXELS)) {
            isr_fired = false;
            while ((ARM_DWT_CYCCNT - cyclesLastHsync) < (cyclesNextPixel)) {}
            pixel = PINS_COLOR;
            cyclesCurrent = ARM_DWT_CYCCNT;
            cyclesRelativeHsync = cyclesCurrent - cyclesLastHsync;

            //calculate pixel position and deviation from ideal sampling point
            currentPixel = cycleToPixel[cyclesRelativeHsync];

            //Discard if unexpected pixel
            if (currentPixel != expectedPixel) {
              isr_fired = true;
            }

            //Discard if too late
            else if ((cyclesRelativeHsync) > pixelToCycleLimit[currentPixel] ) {
              isr_fired = true;
            }

            expectedPixel = currentPixel + 1;

            if (!isr_fired && currentPixel <= PIXELS_350_LINES) {
              //Write pixel
              framebuffer[pixelToFrameBuffer[currentLine] + currentPixel]  = pixelTranslation[EGA][pixel];
              //Uncomment to adjust presampling
              /*
                Serial.println(cyclesRelativeHsync, DEC);
                Serial.println(currentPixel, DEC);
                Serial.println(currentLine, DEC);
                Serial.println(cyclesRelativeHsync - pixelToCyclePresample[currentPixel], DEC);
                Serial.println(cyclesRelativeHsync - pixelToCycle[currentPixel], DEC);
                Serial.println("");
              */
              //Serial.println(pixel,BIN);
              //framebuffer[pixelToFrameBuffer2[currentPixel][currentLine]]  = pixelTranslation[EGA][pixel];
            }

            if (currentPixel >= PIXELS_350_LINES) {
              //End of line
              inputState = STATE_WAIT_HSYNC;
              if (currentLine == LINES_350_LINES)   {
                //End of screen
                inputState = STATE_WAIT_VSYNC;
                lastVsync = digitalReadFast(PIN_VS);
              }
            }
            cyclesNextPixel = pixelToCyclePresample[expectedPixel];
          }
        }

        else if (inputState == STATE_WAIT_VSYNC)  {

          while (inputState == STATE_WAIT_VSYNC)  {

            boolean hsOK = false;
            int countValidHS = 0;
            while (!hsOK) {

              isr_fired = false;
              levelHsync = digitalReadFast(PIN_HS);
              cyclesCurrent = ARM_DWT_CYCCNT;

              if (!levelHsync  && levelHsync != lastHsync ) {
                //HSYNC DETECTED

                if (isr_fired) {
                  countValidHS = 0;
                } else if (countValidHS == 1 && ((cyclesCurrent - cyclesLastHsync < MIN_CYCLES_HSYNC_350_COLOR * 0.90) || (cyclesCurrent - cyclesLastHsync > MIN_CYCLES_HSYNC_350_COLOR * 1.10) )) {
                  currentMode = 0;
                  sync = false;
                  inputState = STATE_WAIT_HSYNC;

#ifdef DEBUG
                  Serial.println("Out of sync HS length");
#endif
                  break;
                } else if (countValidHS == 1) {
                  hsOK = true;
                } else {
                  countValidHS = 1;
                  cyclesLastHsync = cyclesCurrent;
                }
              }

              lastHsync = levelHsync;
            }

            //Time to sample
            levelVsync = digitalReadFast(PIN_VS);

            if (levelVsync && levelVsync != lastVsync ) {
              cyclesLastVsync = cyclesCurrent;
              cyclesLastHsync = cyclesCurrent - CYCLES_HSYNC_HIRES_350_COLOR;
              inputState = STATE_WAIT_HSYNC;
              firstHsync = true;
              currentLine = -BLANK_LINES_SCREEN_350_COLOR;
              testCycle = !testCycle;
#ifdef DEBUG

              uptime++;
              if ( uptime % 60 == 0)
              {
                Serial.print("Temperature:");
                Serial.print(tempmonGetTemp(), 1);
                Serial.print("  Uptime:");
                Serial.println(uptime / 60);
              }


#endif

              //We have some free clock cycles to process OSD
              osd(NAME, vga);

            } // else if ( (cyclesCurrent - cyclesLastVsync) > MAX_CYCLES_VSYNC_350_COLOR ) {
            //sync = false;
            //}
            lastVsync = levelVsync;
          }
        }
      }


      //Restart Teensy if out-of-sync

      //Restart vga lib doesn't seem to work well, so if out of sync we restart the entire micro
      if (!sync && vgaInitialized) {
        //https://forum.pjrc.com/threads/24304-_reboot_Teensyduino()-vs-_restart_Teensyduino()
#ifdef DEBUG
        Serial.println ("Restarting....");
#endif
        vga.end();
        delay(100);
        CPU_RESTART;

        //Restart doesn't even work reliably under teensyduino<1.51... wtf???

      }


    }
  }
}




//Show on-screen display information and process button pushes
FASTRUN void osd(String modeName, VGA_T4 vga) {

  //Show OSD if enabled
  if (osdEnabledCycles) {
    osdEnabledCycles--;
    vga.get_frame_buffer_size(&fb_width, &fb_height);
    String  message = modeName + " Hsync pixel alignment: " + cyclesFirstPixelToUse;
    if (fb_height < 640) {
      message = modeName + " Px. align: " + cyclesFirstPixelToUse;
    }
    if (displayStoredMessage) {
      vga.drawText(10, (fb_height - 50), "                                                           " , BLACK, BLACK, false);
      message = "SETTINGS STORED";
    }

    vga.drawText(10, (fb_height - 50), message.c_str() , LIGHT_BLUE, BLUE, false);
    if (osdEnabledCycles == 0) {
      displayStoredMessage = false;
      vga.drawText(10, (fb_height - 50), "                                                           " , BLACK, BLACK, false);
    }
  }

  //Read buttons states
  boolean stateButtonPlus = digitalReadFast(PIN_PLUS);
  boolean stateButtonMinus = digitalReadFast(PIN_MINUS);
  boolean stateButtonStore = digitalReadFast(PIN_STORE);
  boolean stateButtonReset = digitalReadFast(PIN_RESET);

  //Check button pushes
  int fb_stride = vga.get_fb_stride();
  if (!stateButtonPlus && stateButtonPlusPrevious) {
    //pushed plus
    osdEnabledCycles = 180;
    pushedPlusCycles = 0;
    cyclesFirstPixelToUse += 1;

    buildLookupTables(cyclesFirstPixelToUse,
                      fb_stride
                     );
  } else if (!stateButtonPlus && !stateButtonPlusPrevious) {
    //Continue pushing plus
    pushedPlusCycles++;
    osdEnabledCycles = 180;
    if (pushedPlusCycles > 60) {
      cyclesFirstPixelToUse += 1;
      buildLookupTables(cyclesFirstPixelToUse,
                        fb_stride
                       );
    }

  } else if (!stateButtonMinus && stateButtonMinusPrevious) {
    //pushed minus
    pushedMinusCycles = 0;
    osdEnabledCycles = 180;
    cyclesFirstPixelToUse -= 1;

    buildLookupTables(cyclesFirstPixelToUse,
                      fb_stride
                     );
  } else if (!stateButtonMinus && !stateButtonMinusPrevious) {
    //Continue pushing minus
    osdEnabledCycles = 180;
    pushedMinusCycles++;
    if (pushedMinusCycles > 60) {
      cyclesFirstPixelToUse -= 1;
      buildLookupTables(cyclesFirstPixelToUse,
                        fb_stride
                       );
    }
  }  else if (!stateButtonStore && stateButtonStorePrevious) {
    //pushed store
    pushedStoreCycles = 0;
    osdEnabledCycles = 180;


  } else if (!stateButtonStore && !stateButtonStore) {
    //Continue pushing store
    osdEnabledCycles = 180;
    pushedStoreCycles++;
    if (pushedStoreCycles == 120) {
      //Store settings
      displayStoredMessage = true;
      if (currentMode == MODE_200_COLOR) {
#ifdef DEBUG
        Serial.println("Updating EEPROM value");
#endif
        EEPROMWriteInt(EEPROM_CYCLES_FIRST_PIXEL_MODE_200_COLOR_USER_DEFINED, cyclesFirstPixelToUse);
      } else if (currentMode == MODE_200_COLOR_HR) {
#ifdef DEBUG
        Serial.println("Updating EEPROM value");
#endif
        EEPROMWriteInt(EEPROM_CYCLES_FIRST_PIXEL_MODE_200_COLOR_HR_USER_DEFINED, cyclesFirstPixelToUse);
      } else if (currentMode == MODE_350_COLOR) {
#ifdef DEBUG
        Serial.println("Updating EEPROM value");
#endif
        EEPROMWriteInt(EEPROM_CYCLES_FIRST_PIXEL_MODE_350_COLOR_USER_DEFINED, cyclesFirstPixelToUse);
      }
    }
  }  else if (!stateButtonReset && stateButtonResetPrevious) {
    //pushed store
    pushedResetCycles = 0;
    osdEnabledCycles = 180;


  } else if (!stateButtonReset) {
    //Continue pushing store
    osdEnabledCycles = 180;
    pushedResetCycles++;
    if (pushedResetCycles == 120) {
      //Reset settings
      Serial.println ("Erasing EEPROM....");
      EEPROMWriteInt(EEPROM_CYCLES_FIRST_PIXEL_MODE_200_COLOR_HR, 0);
      EEPROMWriteInt(EEPROM_CYCLES_FIRST_PIXEL_MODE_200_COLOR, 0);
      EEPROMWriteInt(EEPROM_CYCLES_FIRST_PIXEL_MODE_350_COLOR, 0);
      EEPROMWriteInt(EEPROM_CYCLES_FIRST_PIXEL_MODE_350_MONO, 0);
      EEPROMWriteInt(EEPROM_CYCLES_FIRST_PIXEL_MODE_200_COLOR_HR_USER_DEFINED, 0);
      EEPROMWriteInt(EEPROM_CYCLES_FIRST_PIXEL_MODE_200_COLOR_USER_DEFINED, 0);
      EEPROMWriteInt(EEPROM_CYCLES_FIRST_PIXEL_MODE_350_COLOR_USER_DEFINED, 0);
      EEPROMWriteInt(EEPROM_CYCLES_FIRST_PIXEL_MODE_350_MONO_USER_DEFINED, 0);
      String resetMessage = "ERASING ALL SETTINGS AND REBOOTING";
      vga.drawText(10, (fb_height - 50), "                                                           " , BLACK, BLACK, false);
      vga.drawText(10, (fb_height - 50), resetMessage.c_str() , LIGHT_BLUE, BLUE, false);
      delay(3000);
      sync = false;
      currentMode = 0;
    }
  } else if (stateButtonReset && !stateButtonResetPrevious) {
    //reset
    sync = false;
    currentMode = 0;
  }


  stateButtonPlusPrevious = stateButtonPlus;
  stateButtonMinusPrevious = stateButtonMinus;
  stateButtonStorePrevious = stateButtonStore;
  stateButtonResetPrevious = stateButtonReset;
}






//Utility functions from some random guy named vizfxca for reading and writing EEPROM values


//This function will write a 2 byte integer to the eeprom at the specified address and address + 1
void EEPROMWriteInt(int p_address, int p_value)
{
  byte lowByte = ((p_value >> 0) & 0xFF);
  byte highByte = ((p_value >> 8) & 0xFF);

  EEPROM.write(p_address, lowByte);
  EEPROM.write(p_address + 1, highByte);
}


//This function will read a 2 byte integer from the eeprom at the specified address and address + 1
unsigned int EEPROMReadInt(int p_address)
{
  byte lowByte = EEPROM.read(p_address);
  byte highByte = EEPROM.read(p_address + 1);

  return ((lowByte << 0) & 0xFF) + ((highByte << 8) & 0xFF00);
}
