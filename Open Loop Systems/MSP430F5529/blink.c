/* --COPYRIGHT--,BSD_EX
 * Copyright (c) 2012, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *******************************************************************************
 * 
 *                       MSP430 CODE EXAMPLE DISCLAIMER
 *
 * MSP430 code examples are self-contained low-level programs that typically
 * demonstrate a single peripheral function or device feature in a highly
 * concise manner. For this the code may rely on the device's power-on default
 * register values and settings such as the clock configuration and care must
 * be taken when combining code from several examples to avoid potential side
 * effects. Also see www.ti.com/grace for a GUI- and www.ti.com/msp430ware
 * for an API functional library-approach to peripheral configuration.
 *
 * --/COPYRIGHT--*//*
/*
Matt Mammarelli
9/18/17
ECE 09342-2
*/

//Open Loop MSP430F5529
//Receives a temperature over UART from 32 to 75, converts to a pwm, sends the pwm over GPIO pin 1.2
//Receives temperature from ADC12, puts into temp 
//Transmits temperature over UART when Timer B ISR fires, about every 1 second
//Will convert value from LM35 into Celcius before transmitting


#include <msp430f5529.h>
#include <math.h>

int pwm=0; //pwm value
int temp=0;//hold temperature received from ADC
int tempCel=0;//temperature received from UART
float tempC = 0;//temp converted to celsius, will be transmitted over UART
float voltage = 0;//used in conversion to celsius

void main(void)
{

  //stop watchdog timer
  WDTCTL = WDTPW + WDTHOLD;

  //uart **************************************************************************************************
  // P3.3, P3.4 transmit/receive
  P3SEL = BIT3+BIT4;
  // Put state machine in reset
  UCA0CTL1 |= UCSWRST;
  // SMCLK
  UCA0CTL1 |= UCSSEL_2;
  // 1MHz 9600 baud
  UCA0BR0 = 6;
  // 1MHz 9600
  UCA0BR1 = 0;
  //sets m control register
  UCA0MCTL = UCBRS_0 + UCBRF_13 + UCOS16;
  //sets control register
  UCA0CTL1 &= ~UCSWRST;
  //enable interrupt
  UCA0IE |= UCRXIE;
  //*******************************************************************************************************

  //adc ***********************************************************************************************

    ADC12CTL0 = ADC12SHT02 + ADC12ON;         // Sampling time, ADC12 on
    ADC12CTL1 = ADC12SHP;                     // Use sampling timer
    ADC12IE = 0x01;                           // Enable interrupt
    ADC12CTL0 |= ADC12ENC;
    P6SEL |= 0x01;                            // P6.0 ADC option select


  //**********************************************************************************************

  //rgb pwm *****************************************************************************************

  // P1.2  output
  P1DIR |= BIT2;

  // P1.2 select GPIO
  P1SEL |= BIT2;

  // PWM Period about 1khz
  TA0CCR0 = 1024;

  // CCR1 reset/set
  TA0CCTL1 = OUTMOD_7;



  // SMCLK, up mode, clear TAR
  TA0CTL = TASSEL_2 + MC_1 + TACLR;

  //***************************************************************************************************

  //timer ******************************************************************************************

  //TA0CTL = Timer A0 Control
      //TASSEL_1 Timer_A clock source select = 01 ACLK 32k
      //MC_1 Up Mode
      TB0CTL=TBSSEL_1+MC_1; //UP MODE


      TB0CCTL0 = 0x10; //Timer A0 in compare mode
      TB0CCR0=30000;
//***************************************************************************************************

  //adc
  while (1)
    {
      ADC12CTL0 |= ADC12SC;                   // Start sampling/conversion
      __bis_SR_register(LPM0_bits + GIE);     // LPM0, ADC12_ISR will force exit
      __no_operation();                       // For debugger
    }
}

//Port 1 ISR
#pragma vector = TIMER0_B0_VECTOR
__interrupt void TimerB(void) //double __
{
//transmits celsius temerature every second
UCA0TXBUF = tempC;

}


//uart interrupt vector
#pragma vector=USCI_A0_VECTOR
__interrupt void USCI_A0_ISR(void)
{
  switch(__even_in_range(UCA0IV,4))
  {
  case 0:break;   // Vector 0 - no interrupt
  case 2:{
      while (!(UCA0IFG&UCTXIFG));  // USCI_A0 TX buffer check

      tempCel = UCA0RXBUF;


      //temperature ranges
      //pwm set to linear functions based on excel graphs
      if (tempCel == 32){
          pwm = 0xFF;
      }
      else if (tempCel >32 && tempCel <=40){
          pwm = (-6.375*tempCel)+409;
      }
      else if (tempCel >40 && tempCel <=46 ){
          pwm = (-10.2*tempCel)+520.2;
      }
      else if (tempCel > 46 && tempCel <=75){
          pwm = (-1.7586*tempCel)+131.9;
      }


      TA0CCR1 = pwm* 4;


      break;

  }

  case 4:break;    // Vector 4 - TXIFG
  default: break;
  }
}

//adc12 interrupt vector
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector = ADC12_VECTOR
__interrupt void ADC12_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(ADC12_VECTOR))) ADC12_ISR (void)
#else
#error Compiler not supported!
#endif
{
  switch(__even_in_range(ADC12IV,34))
  {
  case  0: break;                           // Vector  0:  No interrupt
  case  2: break;                           // Vector  2:  ADC overflow
  case  4: break;                           // Vector  4:  ADC timing overflow
  case  6:                                  // Vector  6:  ADC12IFG0

  {
      temp = ADC12MEM0;

      voltage = temp* 0.00080566; //3.3/2^12 = .00080566
      tempC = voltage / 0.01;

     


    __bic_SR_register_on_exit(LPM0_bits);   // Exit active CPU

  }
  case  8: break;                           // Vector  8:  ADC12IFG1
  case 10: break;                           // Vector 10:  ADC12IFG2
  case 12: break;                           // Vector 12:  ADC12IFG3
  case 14: break;                           // Vector 14:  ADC12IFG4
  case 16: break;                           // Vector 16:  ADC12IFG5
  case 18: break;                           // Vector 18:  ADC12IFG6
  case 20: break;                           // Vector 20:  ADC12IFG7
  case 22: break;                           // Vector 22:  ADC12IFG8
  case 24: break;                           // Vector 24:  ADC12IFG9
  case 26: break;                           // Vector 26:  ADC12IFG10
  case 28: break;                           // Vector 28:  ADC12IFG11
  case 30: break;                           // Vector 30:  ADC12IFG12
  case 32: break;                           // Vector 32:  ADC12IFG13
  case 34: break;                           // Vector 34:  ADC12IFG14
  default: break;
  }
}









