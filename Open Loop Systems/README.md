# Lab 6: Open Loop Systems
Believe it or not, up to this point, any time that you have wanted to control your LED color or brightness so far, you have been attempting to control an Open Loop System. Basically, when in your code you state that you want a certain brightness or even a duty cycle, you are going on blind faith that the output is actually what it is supposed to be. If something seemed off, you probably went back into the code and tweaked some values. In the case of actual Systems and Control Theory, you are the feedback loop, providing some corrective signal to the system to help obtain a closer output, and we will deal with this in the Milestone. For now, we need to focus on system modeling getting a system to a desirable state. For this lab, you will be attempting to keep a voltage regulator within a specific temperature range using a DC fan which you will have control over. For this part to be a success, you need to figure out what is the minimum fan speed you need to cool off the regulator so that is stays operational.

## Voltage Regulator
You will need to take a 5V regulator from the back of the lab and drop the output across a 100 ohm power resistor (1 watt should do the trick). The input to the voltage regulator will be between 15-20V. I am giving you a range because when it is dropping a ton of voltage, it may not be able to cool it off enough with just a fan. Most of the voltage regulators in the back will have a tab on the top which we can place a thermistor to. If provided, you can use that tab, or place a through-hole thermistor making contact to the component on your board.

### Regulator Voltage
![alt text](images/regulator.jpg "regulator")


## Fan Control
It will be up to you, the engineer, to decide which method you want to use to control the DC fan. Most of these fans run off of 5V, and as such can not directly be driven by your microcontroller. Depending on the type of fan you use, some can take in a PWM control signal, others will need to have the voltage be modified. Since we are not providing you with any mechanical mounts, you are free to place the fan in whatever orientation you wish, so long as it is safe to operate.

### Room Temperature
![alt text](images/roomTemp.jpg "roomtemp")

### Fan Setup
![alt text](images/fanSetup.jpg "Fansetup")


## Temperature Reading
It would be really useful to see what the temperature of your system is so you can determine the performance of your system. This can be done either by displaying the current temperature over a display, passing the information over UART, or other ways as well. Remember that UART is Asynchronous, meaning that you can send information whenever you would like from your controller back to a PC, it doesn't have to be reactionary. If you used MATLAB in Lab 5, you could even plot the temperature over time which could be extremely useful in figuring out whether your system is actually doing something. 



## System Modeling
For starters, you need to figure out with your fan at MAX what the temperature that the voltage regulator reaches. Your thermistors/PTATs/Whatever else you want to use to measure temperature may not be calibrated, so your results may be consistently off with reality, but for now that is ok. After figuring this out, in increments of about 5C, see what fan speed you need to have to maintain that temperature. Do this until your regulator gets down to about 30C-40C, keeping a record of what your applied Duty Cycles or voltages were. Then using this information, attempt to find a transfer function between the applied input and the resulting temperature to model the system behavior. A simple way to do this is in MATLAB or Excel to plot your applied input on the x-axis, and your steady state temperature on your y-axis and attempt a line fit.

![alt text](images/tempDutyGraph.png "tempduty")


## MSP430F5529 Code:

```c
/*
Matt Mammarelli
9/18/17
ECE 09342-2
*/

//Open Loop Control MSP430F5529
//Receives a duty cycle over UART, sends the pwm over GPIO pin 1.2
//Receives temperature from ADC12, puts into array of values 
//Transmits temperature over UART when Timer B ISR fires, about every 1 second



#include <msp430f5529.h>


int pwm=0; //holds UART values for current rgb
int temp[10]; //array to hold temperatures
int count =0; //iterates through temperature array

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
      TB0CCR0=32000;
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
UCA0TXBUF = temp[count];

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


      pwm = UCA0RXBUF;
      // CCR1 PWM duty cycle
      TA0CCR1 = pwm * 4;
      //UCA0TXBUF = temp;

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


      temp[count] = ADC12MEM0; //changes duty cycle
	  //iterates count for temp array
      if(count<10){
          count++;
      }
      else{
          count=0;
      }


    __bic_SR_register_on_exit(LPM0_bits);   // Exit active CPU
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



```

## Open Loop Control System
You then need to use this information to make a final open loop control system where a user can state what temperature they want the regulator to be maintained at, and the microcontroller will basically calculate/look up what it needs to set the fan to. Do not over complicate this and make it some elaborate system. All this needs to do is some math and set a duty cycle or voltage, and display in some capacity the current temperature of the system as you are measuring it.










## Deliverables
Your README needs to contain schematics of your system, the plot of the plot of the temperature and input voltages at the 5C steps, and a brief talk about why you chose the processor you did along with the control technique for the fan. As always, you should include a brief description of the code you generated to run the experiment. You need to also include information on how to use your control software, including what inputs it is expecting and what range of values you are expecting. At this time you are not going to need to user-proof the system, but you will for the milestone, so keep it in the back of your head.

### What your README and code doesn't need
For starters, note the fact I ask you to do this with only one board. You also do not need to give me all of your code in the README, just tell me functionally what is going on along with showing off any functions you may have made.

Your code *DOES NOT* need to perform any sort of closed loop control. Save that for the milestone. This means that your system does not need to try to actively change the fan speed without your help. You are going to essentially make your microcontroller one big Convert-o-Box to turn a desired temperature into a controllable signal, and then be able to read a temperature.