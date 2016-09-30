/*
 * ArduinoExchange
 * 
 * Based on the "Decadic Dial Decoder and Switching Controller" by Beau Walker from bjshed.better-than.tv. Emulates
 * a telephone exchange, allowing old pulse dialing phones to call each other.
 * 
 * Original project at http://bjshed.better-than.tv/projects/electronics/rotary_arduino_exchange/
 * 
 * This sketch extends the original project by allowing more than 2 phones (yet to be tested!). It also adds busy
 * and ringback tones. 
 * 
 * The ringer generator code is also different. It supports a PCR-SIN03V12F20-C ring generator which generates the 90VAC
 * ring voltage. This device is switched on and off via an "inhibit" signal (which is just 5V) from the Arduino.
 * 
 * This sketch uses a Microview to display infomation about the exchange's status.
 * 
 */

#include <MicroView.h>

const int lines = 2;

// define the phone numbers
const char * numbers[lines] = {"440", "441"};

//loop detection for A & B phones
const int      loops[lines] = {A1, A0};

//Relay Switch for Ring / Speech
const int      rings[lines] = {3, 2};


// ringing generator inhibit
const int ring_gen_inhibit = 6;

//tone output pin - this generates the sqaure wave that feeds the bandpass filter
int toneOut = 5;

const int dialling_timeout = 10000;


int needToPrint = 0;
int count;
int lastState = LOW;
int trueState = LOW;
long lastStateChangeTime = 0;

boolean offhook = boolean();
boolean allowdialtone = boolean();

int offhook_line;


// constants

int dialHasFinishedRotatingAfterMs = 100;
int debounceDelay = 10;
String diallednumber = String();
long timeout = millis(); //must be long to fit all of that milli goodness over time.
long ring = millis();

void setup()
{
  pinMode(ring_gen_inhibit, OUTPUT);
  digitalWrite(ring_gen_inhibit, HIGH);

  pinMode(toneOut, OUTPUT);

  offhook = false;
  allowdialtone = true;

  for (int i = 0; i < lines; i++) {
    pinMode(loops[i], INPUT_PULLUP);
    pinMode(rings[i], OUTPUT);
  }

  Serial.begin(9600);
  Serial.println("Arduino Exchange");

  uView.begin();              // start MicroViewuView.clear(PAGE);
  uView.clear(PAGE);
  uView.setCursor(10, 0);
  uView.print("Arduino");
  uView.setCursor(8, 10);
  uView.print("Exchange");
  uView.display();
  uView.setCursor(19, 25);
  uView.print("Idle");
  uView.display();

  delay(2000);
}

/*
 * Tonescript: 400@-19,425@-19,450@-19;10(* /0/1+2+3)
 */
void dialtone()
{
  if  (allowdialtone == true) {
    int freq1 = 400;
    int freq2 = 425;
    int freq3 = 450;
    int duration_divider = 15;

    // to "combine" the two tones we play them idividually, one after the other, really quickly (every duration_divider ms)
    tone(toneOut, freq1, duration_divider);
    delay(duration_divider);
    tone(toneOut, freq2, duration_divider);
    delay(duration_divider);
    tone(toneOut, freq3, duration_divider);
    delay(duration_divider);

    //reset timout for dialing too
    timeout = millis();
  }
}

/*
 * Tonescript: 425@-19;10(.375/.375/1)
 */
void busytone()
{
  int frequency = 425;
  int on_duration = 375;
  int off_duration = 375;

  tone(toneOut, frequency, on_duration);
  delay(on_duration);
  delay(off_duration);
}

/*
 * Tonescript: 400@-16,435@-17;*(.4/.2/1+2,.4/.2/1+2,2/0/0)
 */
void ringbacktone()
{
  int frequency1 = 425;
  int frequency2 = 435;
  
  int on_duration = 400;
  int off_duration = 200;

  int pause_duration = 2000;

  int duration_divider = 15;

  // parts one and two are identical, 400ms on, 200ms off. To "combine" the two tones we play them idividually, one after the other, really quickly (every duration_divider ms)
  for(int i=0; i<2; i++) {
    for(int j=0; j<on_duration; j=j+duration_divider) {
      // freq 1
      tone(toneOut, frequency1, duration_divider);
      delay(duration_divider);
    
      // freq 2
      tone(toneOut, frequency2, duration_divider);
      delay(duration_divider);
    }
  
    delay(off_duration);
  }

  // part three - just a pause...
  delay(pause_duration);
}

void trigger_ringer()
{
  digitalWrite(ring_gen_inhibit, LOW);
  delay(100);
  digitalWrite(ring_gen_inhibit, HIGH);
}


void ringer()
{
  //stop dialtone whilst ringing
  allowdialtone = false;


  // Aussie ring tone
  // 500ms on 250ms off 1000ms space.
  //
  if (millis() - ring <= 400) {
    // ring phone for 400ms
    trigger_ringer();
  }

  if ((millis() - ring >= 600) && (millis() - ring <= 1000)) {
    //ring again after 250ms break
    trigger_ringer();
  }

  if (millis() - ring >= 3000) {
    //ring again after 2000ms break
    ring = millis();
  }

  if ((millis() - ring >= 1000) && (millis() - ring <= 3000)) {
    ringbacktone();
  }
}


/*
 * Based on http://www.instructables.com/id/Interface-a-rotary-phone-dial-to-an-Arduino/step4/Develop-the-code/
 */
void decodenumber()
{
  //if number is 3 numbers then check if it's correct
  while (diallednumber.length() != 3 ) {

    //check for digit
    int reading = digitalRead(loops[offhook_line]);


    if ((millis() - timeout) >= dialling_timeout) {
      //if timeout reached clear dialled numbers
      timeout = millis();
      diallednumber = "";
      lastState = LOW;

      // update display
      Serial.println("Dialling timeout, try again");
      
      while (digitalRead(loops[offhook_line]) == LOW) {
        //generate busytone
        busytone();
      }

      return;
    }

    if ((millis() - lastStateChangeTime) > dialHasFinishedRotatingAfterMs) {
      // the dial isn't being dialed, or has just finished being dialed.
      if (needToPrint) {
        // if it's only just finished being dialed, we need to send the number down the serial
        // line and reset the count. We mod the count by 10 because '0' will send 10 pulses.
        diallednumber = diallednumber + count % 10, DEC;
        
        Serial.println("Dialled number: " + diallednumber);
        
        uView.clear(PAGE);
        uView.setCursor(0, 10);
        uView.print("Dialing...");
        uView.setCursor(20, 30);
        uView.print(diallednumber);
        uView.display();
        
        needToPrint = 0;
        count = 0;
        timeout = millis();
      }
    
    }

    if (reading != lastState) {
      lastStateChangeTime = millis();
    }
    
    if ((millis() - lastStateChangeTime) > debounceDelay) {
      // debounce - this happens once it's stablized
      if (reading != trueState) {
        // this means that the switch has either just gone from closed->open or vice versa.
        trueState = reading;
        if (trueState == HIGH) {
          // increment the count of pulses if it's gone high.
          count++;
          needToPrint = 1; // we'll need to print this number (once the dial has finished rotating)

        }
      }
    }
    
    lastState = reading;

    // detect if the phone goes back on-hook (i.e. goes HIGH and stays HIGH)
    if(lastState == HIGH && (millis() - lastStateChangeTime) > 2000) {
      timeout = millis();
      diallednumber = "";
      lastState = LOW;
      
      Serial.println("hungup without dialling");
      return;
    }
  }

  checknumber();
}

void checknumber() {

  int dialled_line = -1;
  for (int i = 0; i < lines; i++) {
    if (diallednumber == numbers[i]) {
      dialled_line = i;
    }
  }

  if (dialled_line > -1 && offhook_line != dialled_line) {
    String msg = numbers[offhook_line];
    msg.concat(" -> ");
    msg.concat(numbers[dialled_line]);
    
    Serial.println(msg);
    Serial.println("Ringing...");
        
    uView.clear(PAGE);
    uView.setCursor(0, 10);
    uView.print(msg);
    uView.setCursor(0, 20);
    uView.print("Ringing...   ");
    uView.display();

    //set calling party relay on
    digitalWrite(rings[offhook_line], HIGH);

    while (digitalRead(loops[dialled_line]) == HIGH) {
      // ring until A picks up
      ringer();

      // drop call if caller hangs up
      if (digitalRead(loops[offhook_line]) == HIGH) {
        break;
      }
    }

    delay(100);
    //digitalWrite(rings[offhook_line], LOW);
    digitalWrite(rings[dialled_line], HIGH);

    while (digitalRead(loops[offhook_line]) == LOW) {
      //stay here unitl call is completed.
      Serial.println("Talking...");
        
      uView.setCursor(0, 20);
      uView.print("Talking...   ");
      uView.display();
    
      delay (10);
    }

    Serial.println("* Hungup *");
    
    uView.setCursor(0, 20);
    uView.print("* Hungup *");
    uView.display();

    digitalWrite(rings[dialled_line], LOW);
    delay (500);
  }

  while (digitalRead(loops[offhook_line]) == LOW) {
    //generate busytone
    busytone();
  }

}

void loop()
{

  for (int i = 0; i < lines; i++) {
    while (digitalRead(loops[i]) == LOW && offhook == false) {
      offhook_line = i;
      offhook = true;

      String msg = "Line off-hook: ";
      msg.concat(offhook_line);

      Serial.println(msg);
      
      uView.clear(PAGE);
      uView.setCursor(0, 10);
      uView.print(msg);
      uView.display();

      //set calling party relay on
      digitalWrite(rings[offhook_line], HIGH);

      while (digitalRead(loops[offhook_line]) == LOW) {
        //generate dialtone
        dialtone();
      }
      
      // reset ready to try again
      diallednumber = "";
      timeout = millis();
      ring = millis();
      lastState = LOW;
      
      decodenumber();
    }
  }

  if(offhook != false) { //ensures this is only printed once after phones are back on-hook, otherwise microview flickers with constant looping
    uView.clear(PAGE);
    uView.setCursor(10, 0);
    uView.print("Arduino");
    uView.setCursor(8, 10);
    uView.print("Exchange");
    uView.display();
    uView.setCursor(19, 25);
    uView.print("Idle");
    uView.display();
  }

  offhook = false;
  allowdialtone = true;

  //set calling party relay off
  for (int i = 0; i < lines; i++) {
    digitalWrite(rings[i], LOW);
  }

}


