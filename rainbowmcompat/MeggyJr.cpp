/*
  RainbowMCompat - MeggyJr-compatible library for Rainbowduino
  
  Based on MeggyJr v1.31:
  
  MeggyJr.cpp - Meggy Jr RGB library for Arduino
  Version 1.3 - 1/01/2009       http://www.evilmadscientist.com/
  Copyright (c) 2008 Windell H. Oskay.  All right reserved.

    This library is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this library.  If not, see <http://www.gnu.org/licenses/>.
	
	Thanks to Arthur J. Dahm III and Jay Clegg for code written for the Peggy 2.0,
	which was adapted to make this library.  
	
*/

/******************************************************************************
 * Includes
 ******************************************************************************/

extern "C" {
#include <inttypes.h>
#include <avr/interrupt.h>
#include <avr/io.h> 
#include <wiring.h> 
} 

#include "MeggyJr.h"
#include "rainbow.h" 

byte MeggyJr::MeggyFrame[DISP_BUFFER_SIZE];

byte MeggyJr::AuxLEDs;
byte MeggyJr::SoundAllowed;	 
byte MeggyJr::SoundEnabled;	

byte MeggyJr::currentCol;
byte MeggyJr::currentBrightness;
byte* MeggyJr::currentColPtr;
		 
//unsigned long MeggyJr::SoundStopTime;	

unsigned int MeggyJr::ToneTimeRemaining;

#ifdef timingtest
unsigned int MeggyJr::testTime;
#endif

/* Rainbowduino interrupt service functions, based on Rainbow_CMD_V2_0 from seeedstudio */	
unsigned char rainbowLine,rainbowLevel;

//==============================================================
void shift_24_bit(unsigned char line,unsigned char level)   // display one line by the color level in buff
{
  unsigned char color=0,column=0;
  unsigned char pixel=0;
 	unsigned char pixelOffset;
  byte pixelPtr;
  le_high;	//  Latch enable input
  for(color=0;color<3;color++)//GRB
  {
  	pixelPtr = line*24;
  	// Rainbowduino shift reg is in GRB order, MeggyFrame is in BGR order with one row of each colour
  	switch (color) {
  		case 0: pixelOffset = 8; break; // Green
  		case 1: pixelOffset = 16; break; // Red
  		case 2: pixelOffset = 0; break; //Blue
  	}
  	pixelPtr += pixelOffset;
    for(column=0;column<8;column++)
    {
    	pixel = MeggyJr::MeggyFrame[pixelPtr++];

			// inlined shift_1_bit
      if(pixel>level)   //gray scale,0x0f always light
      {
        shift_data_1;
      }
      else
      {
        shift_data_0;
      }
  		clk_rising; //  clock port down, then up, to shift data into register
		}  		
  }
  le_low;	//  Latch disable input
}



//==============================================================
void open_line(unsigned char line)     // open the scanning line 
{
  switch(line)
  {
  case 0:
    {
      open_line0;
      break;
    }
  case 1:
    {
      open_line1;
      break;
    }
  case 2:
    {
      open_line2;
      break;
    }
  case 3:
    {
      open_line3;
      break;
    }
  case 4:
    {
      open_line4;
      break;
    }
  case 5:
    {
      open_line5;
      break;
    }
  case 6:
    {
      open_line6;
      break;
    }
  case 7:
    {
      open_line7;
      break;
    }
  }
}
//==============================================================
void flash_next_line(unsigned char line,unsigned char level) // scan one line
{
  disable_oe;			//  MBI5168 disable output
  close_all_line; //  all anodes down
  open_line(line); //  anode for this line up
  shift_24_bit(line,level); 
  enable_oe; //  MBI5168 enable output
}


ISR(TIMER2_OVF_vect)          //Timer2  Service 
{ 
  TCNT2 = 0xE7; // TODO GamaTab[rainbowLevel];    // Reset a  scanning time by gamma value table
  flash_next_line(rainbowLine,rainbowLevel);  // sacan the next line in LED matrix level by level.
  rainbowLine++;
  if(rainbowLine>7)        // when have scaned all LEC the back to line 0 and add the level
  {
    rainbowLine=0;
    rainbowLevel++;
    if(rainbowLevel>15)       rainbowLevel=0;
  }
}


void init_timer2(void)               
{
  TCCR2A |= (1 << WGM21) | (1 << WGM20);   //  Fast PWM mode??
  TCCR2B |= (1<<CS22);   // by clk/64
  TCCR2B &= ~((1<<CS21) | (1<<CS20));   // by clk/64
  TCCR2B &= ~((1<<WGM21) | (1<<WGM20));   // Use normal mode
  ASSR |= (0<<AS2);       // Use internal clock - external clock not used in Arduino
  TIMSK2 |= (1<<TOIE2) | (0<<OCIE2B);   //Timer2 Overflow Interrupt Enable
  TCNT2 = 0xE7; // TODO GamaTab[0];
  sei();   
}



void rainbow_init(void)    // define the pin mode
{
	//  All ports for output
  DDRD=0xff;
  DDRC=0xff;
  DDRB=0xff;
  //  output port values set to 0
  PORTD=0;
  PORTB=0;
  init_timer2();  // initial the timer for scanning the LED matrix
}

/******************************************************************************
 * Constructor
 ******************************************************************************/

MeggyJr::MeggyJr(void)			
{

// Initialization routine for Meggy Jr lib and Rainbowduino hardware

	AuxLEDs = 0;
	currentColPtr = MeggyFrame;
	currentCol=0;
	currentBrightness=0;
	
  SoundAllowed = 0;
  SoundEnabled = 0;
   
	MeggyJr::ToneTimeRemaining = 0;
   
	ClearMeggy();

	rainbow_init(); // Rainbowduino init code
  	 
}


/******************************************************************************
 * User API
 ******************************************************************************/
 
// Painfully Slow!  Don't use this, if you can avoid it:
void MeggyJr::ClearMeggy (void)
{
	for (byte i = 0; i < DISP_BUFFER_SIZE; i++)
	{ 
	    MeggyFrame[i]= 0;
	}
}
 
//Set Pixel Color:  use an RGB array to specify the color.
// Very convenient for using color look-up tables.
void MeggyJr::SetPxClr(byte x, byte y, byte *rgb)
{
  byte PixelPtr =  24*x + y;
  MeggyFrame[PixelPtr] = rgb[2];   
  PixelPtr += 8;
  MeggyFrame[PixelPtr] = rgb[1];
  PixelPtr += 8;
  MeggyFrame[PixelPtr] = rgb[0]; 
}
  
 
  
/* 
void MeggyJr::SoundCheck(void)   
  { 
 // Obsolete with current version of library; sounds stop automatically.
 // If your program contains "SoundCheck();" somewhere, please remove it.
}
*/  
   

// Begin sound 
void  MeggyJr::StartTone(unsigned int Tone, unsigned int duration)   
  {
  //NOP
  SoundState(1);
  }
    	
 
byte MeggyJr::GetPixelR(byte x, byte y)
{
  return MeggyFrame[24*x + y + 16]; 
}

byte MeggyJr::GetPixelG(byte x, byte y)
{
  return MeggyFrame[24*x + y + 8]; 
}

byte MeggyJr::GetPixelB(byte x, byte y)
{
  return MeggyFrame[24*x + y]; 
}

// Clear a single pixel.  Not much better than writing "dark" to the pixel.
void MeggyJr::ClearPixel(byte x, byte y)
{
byte PixelPtr =  24*x + y;
MeggyFrame[PixelPtr] = 0;   
PixelPtr += 8;
MeggyFrame[PixelPtr] = 0;
PixelPtr += 8;
MeggyFrame[PixelPtr] = 0; 
}

 
// GetButtons returns a byte with a bit set for 
// each of the buttons that is currently pressed.

byte MeggyJr::GetButtons(void)
{
  return (0); 
}



// Set sound ON or OFF by calling
// SoundState(0) or SoundState(1).

void MeggyJr::SoundState(byte t)
{
	//NOP
   SoundEnabled = 0;
}
  
