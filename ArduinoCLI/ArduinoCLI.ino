/*LED blinking with FreeRTOS and CLI
Designed by Yang Yang Lee
Created : 4th Mar 2026
Last modified : 4th Mar 2026

CLI command via UART
>blink (no parameters given, default blinking at 1Hz)
>blink stop (stop blinking)
>blink freq <value> (non-zero to 500Hz blinking)
>blink period <value> (1 to 60000 ms)
>blink tHigh <value> tLow <value> (custom period for high and low. Arguments can be reversed)
*if the LED stops blinking, it will be restarted upon receiving a valid command.

e.g.:
>blink freq 10 (blink at 10Hz)
>blink tHigh 1 tLow 1000 (spot light style blinking, arguments can be in reverse)
>blink stop (stop blinking after the LED turns off, i.e. suspend task)
>blink (start blinking again at default 1Hz frequency)

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
*/

//FreeRTOS
#if CONFIG_FREERTOS_UNICORE
#define TASK_RUNNING_CORE 0
#else
#define TASK_RUNNING_CORE 1
#endif

#ifndef LED_BUILTIN
#define LED_BUILTIN 4  // Specify the on which is your LED
#endif

// Define two tasks for Blink & CLI processor.
void TaskBlink(void *pvParameters);
void TaskCLI(void *pvParameters);

//function prototyping
void recvWithEndMarker();
void parseData();
void checkParsedData();
int command_blink();
void resume_blink();

boolean newData = false;
String commands;
String args1,args2,args3,args4;
String val1,val2,val3,val4;
const byte numChars = 32;
char receivedChars[numChars],messageFromPC[numChars];
unsigned short got_args, got_vals;

uint32_t blink_delay = 500;  // Delay between changing state on LED pin
uint32_t blink_delay_high_cmd = 500;
uint32_t blink_delay_low_cmd = 500;
boolean blink_run = true;
TaskHandle_t blink_task_handle;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.print("Start running...\n");
  xTaskCreate(
    TaskBlink, "Task Blink"  // A name just for humans
    ,
    2048  // The stack size can be checked by calling `uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);`
    ,
    (void *)&blink_delay  // Task parameter which can modify the task behavior. This must be passed as pointer to void.
    ,
    2  // Priority
    ,
    &blink_task_handle
  );
  xTaskCreate(
    TaskCLI, "Task CLI"  // A name just for humans
    ,
    2048  // The stack size can be checked by calling `uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);`
    ,
    NULL
    ,
    2  // Priority
    ,
    NULL  // Task handle is not used here - simply pass NULL
  );
}

void loop() {
  //do your stuff
}

/*--------------------------------------------------*/
/*---------------------- Tasks ---------------------*/
/*--------------------------------------------------*/

void TaskBlink(void *pvParameters) {  // This is a task.
  uint32_t blink_delay_high = *((uint32_t *)pvParameters);
  uint32_t blink_delay_low = *((uint32_t *)pvParameters);

  // initialize digital LED_BUILTIN on pin 13 as an output.
  pinMode(LED_BUILTIN, OUTPUT);

  for (;;) {                          // A Task shall never return or exit.
    //check for run command
    if(!blink_run){
      vTaskSuspend(NULL);   // suspend itself
    }
    //check for new command
    blink_delay_high = blink_delay_high_cmd;
    blink_delay_low =  blink_delay_low_cmd;
    digitalWrite(LED_BUILTIN, HIGH);  // turn the LED on (HIGH is the voltage level)
    // arduino-esp32 has FreeRTOS configured to have a tick-rate of 1000Hz and portTICK_PERIOD_MS
    // refers to how many milliseconds the period between each ticks is, ie. 1ms.
    vTaskDelay(blink_delay_high);
    digitalWrite(LED_BUILTIN, LOW);  // turn the LED off by making the voltage LOW
    vTaskDelay(blink_delay_low);
  }
}
void TaskCLI(void *pvParameters){
  for (;;) {
    recvWithEndMarker();
    if(newData){
      parseData();
      checkParsedData();
      newData=false;
    }
    vTaskDelay(1);    //must insert a delay to release control to the RTOS scheduler to avoid watchdog timeout
  }
}
void recvWithEndMarker() {
  static byte ndx = 1;
  char endMarker = '\n';
  char rc;
  // if main USB serial got data
  while (Serial.available() > 0 && newData == false) {
    rc = Serial.read();
    if (rc != endMarker) {
      if(ndx==1){receivedChars[0] = ' ';}   //purposely add space to the first char for tokenisation
      receivedChars[ndx] = rc;
      ndx++;
      if (ndx >= numChars) {                //safety to prevent array overflow
        ndx = numChars - 1;
      }
    }
    else {
      receivedChars[ndx] = '\0';            // terminate the string
      ndx = 1;
      newData = true;
    }
  }
}
void parseData() {
    // split the data into its parts
  char * strtokIndx; // this is used by strtok() as an index
  static unsigned short read_count = 0;
  strtokIndx = strtok(receivedChars," ,-");
  while(strtokIndx != NULL){
    strcpy(messageFromPC, strtokIndx);
    strtokIndx = strtok(NULL," ,");
    switch(read_count){
      case 0:
        commands = String(messageFromPC);
        got_args = read_count;
        got_vals = read_count;
        break;
      case 1:
        args1 = String(messageFromPC);
        got_args = 1;
        break;
      case 2:
        val1 = String(messageFromPC);
        got_vals = 1;
        break;
      case 3:
        args2 = String(messageFromPC);
        got_args = 2;
        break;
      case 4:
        val2 = String(messageFromPC); 
        got_vals = 2;
        break;
      case 5:
        args3 = String(messageFromPC);
        got_args = 3;
        break;
      case 6:
        val3 = String(messageFromPC); 
        got_vals = 3;
        break;
      case 7:
        args4 = String(messageFromPC);
        got_args = 4;
        break;
      case 8:
        val4 = String(messageFromPC); 
        got_vals = 4;
        break;
      default:
        break;
    }
    read_count++;
  }
  read_count = 0;
}
void checkParsedData(){
  static int errors;
  errors = 0; //hypothesis
  if(commands == "blink"){
    errors = command_blink();
  }
  else if(commands == "CLEAR"){}
  else if(commands == "HOME"){}
  else if(commands == "STEP"){}
  else{errors = 1;}
  switch(errors){
    case 1: Serial.println(F("Command not recognised."));break;
    case 2: Serial.println(F("Command has no valid input range!"));break;
    case 3: Serial.println(F("Command param not recognised!"));break;
    case 4: Serial.println(F("Value must be bigger than zero!")); break;
    case 5: Serial.println(F("Parameters must be in range!")); break;
    default: break;
  }
}
int command_blink(){
  float x1,x2;
  if(got_args == 0){      //default blink at 1Hz
    blink_delay_high_cmd = 500;
    blink_delay_low_cmd = 500;
    resume_blink();
    return 0;
  }
  else if(got_args == 1){
    if(args1 == "stop"){
      blink_run = false;
      return 0;
    }
    else if(args1 == "freq"){
      if(got_vals == 1){
        x1 = val1.toFloat();
        //check range
        if(x1 == 0){return 4;}
        else if(x1 > 500){return 5;}  //min 1ms delay
        blink_delay_high_cmd = (int)(500.0/x1);
        blink_delay_low_cmd = blink_delay_high_cmd;
        resume_blink();
        return 0;
      }
      else{return 2;}
    }
    else if(args1 == "period"){
      if(got_vals == 1){
        x1 = val1.toFloat();
        //check range
        if(x1 < 1 || x1 > 60000){return 5;}
        blink_delay_high_cmd = (int)(x1);
        blink_delay_low_cmd = blink_delay_high_cmd;
        resume_blink();
        return 0;
      }
      else{return 2;}
    }
    else{return 3;}
  }
  else if(got_args == 2){
    if(got_vals == 2){
      x1 = val1.toFloat();x2 = val2.toFloat();
      if(x1 < 1 || x1 > 60000){return 5;}
      else if(x2 < 1 || x2 > 60000){return 5;}
      else{
        if(args1 == "tHigh" && args2 == "tLow"){
          blink_delay_high_cmd = (int)(x1);
          blink_delay_low_cmd = (int)(x2);
          resume_blink();
          return 0;
        }
        else if(args1 == "tLow" && args2 == "tHigh"){
          blink_delay_high_cmd = (int)(x2);
          blink_delay_low_cmd = (int)(x1);
          resume_blink();
          return 0;
        }
        else{return 3;}
      }
    }
    else{return 2;}
  }
  else{return 2;}
}
void resume_blink(){
  if(!blink_run){
    blink_run = true;
    vTaskResume(blink_task_handle);
  }
}
