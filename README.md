# Arduino-FreeRTOS-CLI-interface
This code demonstrate controlling ESP32 LED blinking via CLI (command line interface) like control. Multiple tasks could be controlled via UART CLI style command

CLI command via serial monitor 
>blink (no parameters given, default blinking at 1Hz)
>
>blink stop (stop blinking)
>
>blink freq (value) (non-zero to 500Hz blinking)
>
>blink period (value) (1 to 60000 ms)
>
>blink tHigh (value) tLow <value> (custom period for high and low. Arguments can be reversed)
>
*if the LED stops blinking, it will be restarted upon receiving a valid command.

e.g.:
>blink freq 10 (blink at 10Hz)
>
>blink tHigh 1 tLow 1000 (spot light style blinking, arguments can be in reverse)
>
>blink stop (stop blinking after the LED turns off, i.e. suspend task)
>
>blink (start blinking again at default 1Hz frequency)
>

Any invalid CLI inputs will throw an error and will not update the current blinking task.

Description:
This is a demo of CLI (command line interface) for controlling ESP32 LED blinking via UART.
You can control the microcontroller as if you are using a Linux CLI. Despite some of the 
processing overhead, it is fun to have advanced control scheme on the microcontroller.
The command could be sent via serial monitor or any preprocessing method (e.g. Python and MATLAB)
as long as UART is concerned.

Two tasks are created in the FreeRTOS for blinking and serial processor. The serial processor will 
update the variable upon a valid CLI input and command the blinking task. More parallel tasks could
be added in the future in a similar fashion, allowing multiple task controls via UART.  
