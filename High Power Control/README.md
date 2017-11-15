# Lab 6: "High Power" Control
Relay and Mosfet Switching circuits can both be used to control higher voltages with lower voltages. 
These devices can help prevent damage to a microcontroller by preventing a direct connection to a load

## Relay Switching
The MSP430G2553 was used to output a PWM signal to the 12V relay in order to control the voltage dropped across a power resistor.
It was found that the turn on voltage where the relay would switch was 7V. When the relay was turned on, the current through the circuit was measured to be 5.2mA.
This is still less that the 6mA current draw limit for the MSP430 so the microcontroller wasn't damaged. 

## MOSFET Switching
The relay was replaced with a MOSFET and the current running through the circuit was 3.4mA. The gate of the MOSFET draws zero current ideally so this contributes to the decrease in current draw.
The MOSFET could also switch faster at a lower turn on voltage compared to the relay.

![alt text](images/lowside.png "Low Side Switch") 

## Best Solution
A MOSFET would be the best solution because there is almost no current draw on the gate from the MSP and because it switches faster since it isn't electromechanical.


