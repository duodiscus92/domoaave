*/*
 * blink.c:
 *	Standard "blink" program in wiringPi. Blinks an LED connected
 *	to the first GPIO pin.
 *
 * Copyright (c) 2012-2013 Gordon Henderson. <projects@drogon.net>
 ***********************************************************************
 * This file is part of wiringPi:
 *	https://projects.drogon.net/raspberry-pi/wiringpi/
 *
 *    wiringPi is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU Lesser General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    wiringPi is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Lesser General Public License for more details.
 *
 *    You should have received a copy of the GNU Lesser General Public License
 *    along with wiringPi.  If not, see <http://www.gnu.org/licenses/>.
 ***********************************************************************
 */

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <wiringPi.h>

// LED Pin - wiringPi pin 0 is BCM_GPIO 17.

#define	LED	23

int gpio, tbas, thaut;

int main (int argc, char **argv)
{
  printf ("MM32LM45 Simulator\n") ;
  if(argc != 3){
    printf("***Erreur : 3 paramètres sont attendus :  n° GPIO, durée de l'état 0 (en ms), durée de l'état 1 (en ms)\n");
    return 1;
  }

/*  if((!isnumber(argv[1])|| !isnumber(argv[2]) ||  (!isnumber(argv[3]) {
    printf("***Erreur : L'un des paramètres n'est pas un nombre");
    return 1;
  }*/

  gpio = atoi(argv[1]); tbas = atoi (argv[2]); thaut = atoi(argv[3]);
 
  if (gpîo == 0 || tbas == 0 || thaut ==0) {
    printf("***Erreur sur les valeurs de un ou plusieurs paramètres\n");
    return 1;
  }

  wiringPiSetup () ;
  pinMode (gpio, OUTPUT) ;

  for (;;)
  {
    digitalWrite (gpio, HIGH) ;	// On
    delay (thaut) ;		// mS
    digitalWrite (gpio, LOW) ;	// Off
    delay (tbas) ;
  }
  return 0 ;
}
