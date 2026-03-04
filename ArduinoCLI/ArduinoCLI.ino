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
>blink HP (toggle to blink on high power LED)
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
#define LED_BUILTIN 33  // Specify the on which is your LED
#define LED_BUILTIN_HP 4  // Specify the on which is your LED
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
String args[4];
String vals[4];
const byte numChars = 32;
char receivedChars[numChars],messageFromPC[numChars];
unsigned short got_args, got_vals;

//command buffer
uint32_t blink_delay = 500;  // Delay between changing state on LED pin
uint32_t blink_delay_high_cmd = 500;
uint32_t blink_delay_low_cmd = 500;
boolean high_power_LED_cmd = false;
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
  boolean hpLED;

  // initialize digital LED_BUILTIN on pin 13 as an output.
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LED_BUILTIN_HP,OUTPUT);

  for (;;) {                          // A Task shall never return or exit.
    //check for run command
    if(!blink_run){
      vTaskSuspend(NULL);   // suspend itself
    }
    //check for new command
    blink_delay_high = blink_delay_high_cmd;
    blink_delay_low =  blink_delay_low_cmd;
    hpLED = high_power_LED_cmd;
    //execute
    if(!hpLED){digitalWrite(LED_BUILTIN, HIGH);}
    else{digitalWrite(LED_BUILTIN_HP, HIGH);}
    // arduino-esp32 has FreeRTOS configured to have a tick-rate of 1000Hz and portTICK_PERIOD_MS
    // refers to how many milliseconds the period between each ticks is, ie. 1ms.
    vTaskDelay(blink_delay_high);
    if(!hpLED){digitalWrite(LED_BUILTIN, LOW);}
    else{digitalWrite(LED_BUILTIN_HP, LOW);}
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
    vTaskDelay(100);    //must insert a delay to release control to the RTOS scheduler to avoid watchdog timeout
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
  int idx;
  strtokIndx = strtok(receivedChars," ,-");
  while(strtokIndx != NULL){
    strcpy(messageFromPC, strtokIndx);
    strtokIndx = strtok(NULL," ,");
    if(read_count==0){
      commands = String(messageFromPC);
      got_args = read_count;
      got_vals = read_count;
    }
    else if(read_count % 2 == 1){ //odd number store args
      idx = (read_count-1)/2;
      args[idx] = String(messageFromPC);
      got_args = idx + 1;
    }
    else{   //even number store vals
      idx = read_count/2 - 1;
      vals[idx] = String(messageFromPC);
      got_vals = idx + 1;
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
  unsigned short args_count;
  int error = 0; //hypothesis
  //special case when no arguments are provided
  if(got_args == 0){      //default blink at 1Hz
    blink_delay_high_cmd = 500;
    blink_delay_low_cmd = blink_delay_high_cmd;
  }
  //when got arguments, proceed to check them
  while(args_count != got_args){
    if(args[args_count] == "stop"){
      blink_run = false;
    }
    else if(args[args_count] == "freq"){
      if(got_vals == got_args){
        x1 = vals[args_count].toFloat();
        if(x1 == 0){error = 4; break;}
        else if(x1 > 500){error = 5; break;}  //min 1ms delay
        blink_delay_high_cmd = (int)(500.0/x1);
        blink_delay_low_cmd = blink_delay_high_cmd;
      }
      else{error = 2;break;}
    }
    else if(args[args_count] == "period"){
      if(got_vals == got_args){
        x1 = vals[args_count].toFloat();
        //check range
        if(x1 < 1 || x1 > 60000){error = 5;break;}
        blink_delay_high_cmd = (int)(x1);
        blink_delay_low_cmd = blink_delay_high_cmd;
      }
      else{error = 2;break;}
    }
    else if(args[args_count] == "tHigh"){
      if(got_vals == got_args){
        x1 = vals[args_count].toFloat();
        if(x1 < 1 || x1 > 60000){error = 5;break;}
        blink_delay_high_cmd = (int)(x1);
      }
      else{error = 2;break;}
    }
    else if(args[args_count] == "tLow"){
      if(got_vals == got_args){
        x1 = vals[args_count].toFloat();
        if(x1 < 1 || x1 > 60000){error = 5;break;}
        blink_delay_low_cmd = (int)(x1);
      }
      else{error = 2;break;}
    }
    else if(args[args_count] == "HP"){
      high_power_LED_cmd = !high_power_LED_cmd; //toggle HPLED
    }
    else{error = 3;break;}
    args_count++;
  }
  //after processing all command, execute if no error
  if(error == 0){resume_blink();}
  return error;
}
void resume_blink(){
  if(!blink_run){
    blink_run = true;
    vTaskResume(blink_task_handle);
  }
}
