/*
 * tach.c
 *
 * Created: 2014-12-11
 * Author: Craig Hollinger
 *
 * This is a simple demonstration using an external interrupt pin and a
 * timer/counter to implement a tachometer.  The pulse input from the 
 * shaft rotation sensor (Hall effect sensor in this case) is applied to
 * the external interrupt pin.  The software assumes there is one pulse
 * per revolution of the shaft.
 *
 * Although the code uses an external interrupt pin, it doesn't actually use
 * the interrupt.  In fact, any I/O pin could be used.  The code just stalls
 * waiting for the pin to change.  While stalled, the microcontroller
 * can't do anything else.  This code could be modified to utilize the
 * interrupt, then the microcontroller could do something else while waiting
 * for a pulse from the shaft.
 *
 * Note:
 * This code may not run under Arduino for varios reasons:
 *    - Arduino utilizes TCC1 in the background (for PWM)
 *    - this code assumes a main processor clock frequency of 20MHz not the 16MHz
 *      of Arduino
 *    - the method main() is defined here, whereas Arduino defines it elsewhere
 *      so the compiler will flag the conflict as an error
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of either the GNU General Public License version 3
 * or the GNU Lesser General Public License version 3, both as
 * published by the Free Software Foundation.
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>
/* LCD driver, you can use your own */
#include "hd44780.h"

/* these values are written to the TCC1 control register to setup the
   timer/counter */
#define TIMER_OFF 0b00000000
/* divide system clock by 1024 -> 51.2usec per tick 
   assumes system clock is 26MHz */
#define TIMER_ON  0b00000101

/* This factor converts the time measured for each revolution of the shaft to
 * RPM.
 * The timer ticks once every 51.2usec (the / 512 factor), 60 revolutions per
 * minute. The 10000000 converts us to seconds.
 */
#define TIMER_TO_TACH (60UL * 10000000UL / 512UL)

int main(void){
/* Variables to store the accumulated count. */
  unsigned int tachTimer;
  unsigned long tachometer;
/* String to store the tachometer value for displaying on the LCD. */
  char tempStr[10];

/* set up the LCD, you replace this with your own driver */
  hd44780_init(&PORTD, PORTD7, &PORTB, PORTB1, &PORTB, PORTB0, 2, 20);

/* setup the external interrupt 0 (INT0) pin
   we won't actually be generating an interrupt with the pin in this case */
  DDRD &= ~_BV(PORTD2);
  EICRA |= _BV(ISC01); /* INT0 generates interrupt on falling edge */

/* setup timer 1 to measure the time between interrupts */
  TCCR1A = 0b00000000; /* reset state, makes TCC1 just a counter that increments */
  TCCR1B = TIMER_OFF; /* start with the counter off */
  TCNT1 = 0; /* start with the counter register zero */

  while(1) /* loop here forever processing Timer 1 */
  {
/* Stall here monitoring the INT0 pin's interrupt flag. When a pulse arrives from
   the shaft, drop out of the while() and execute the code below. */
    while((EIFR & _BV(INTF0)) == 0);
    
    TCCR1B = TIMER_ON;     /* turn on the timer */
    GTCCR |= _BV(PSRSYNC); /* reset the timer pre-scaler */
    EIFR = _BV(INTF0);     /* clear the interrupt flag */
    
/* Stall here again monitoring the INT0 pin's interrupt flag.  When the next
   pulse arrives from the shaft, drop out of the while() and process the
   accumulated count. */
    while((EIFR & _BV(INTF0)) == 0);
    
    TCCR1B = TIMER_OFF;    /* turn off the timer */
    tachTimer = TCNT1;
    TCNT1 = 0;

/* now process the Timer 1 count and write it to the LCD */
    tachometer = TIMER_TO_TACH / (unsigned long)tachTimer;
    ltoa(tachometer, tempStr, 10); /* convert to a string */
    hd44780_clearLine(0);
    hd44780_putstr(tempStr); /* display the string on the LCD */
    EIFR = _BV(INTF0);     /* clear the interrupt flag */
  }
}
