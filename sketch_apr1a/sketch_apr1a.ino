//#include <microWire.h>
#include <GyverOLED.h>
#define DISPARRSTRLEN 2

char dispArr[DISPARRSTRLEN + 1] = { 0, 0, 0 };  //cmd array
char ind_dispArr = 0;                           //used as a pointer to array
char cmdSent = 0;                               //used as cmdSent status for # key -cmd or Play/Stop
int16_t counterT = 0;

// NORTOS FREE LIBRARY IMPLEMMENTATION. NO ANY RTOS NEEDED, YOU WILL SEE
// NORTOS: The simplicity matter! By Aleksei Tertychnyi, 2015

typedef void (*fP)(void);  //pointer type for Queues NORTOS

class fQ {
private:
  int first;
  int last;
  fP* fQueue;
  int lengthQ;
public:
  fQ(int sizeQ);
  ~fQ();
  int push(fP);
  int pull(void);
  void waitOn(unsigned long delay);
};

fQ::fQ(int sizeQ) {  // initialization of Queue
  fQueue = new fP[sizeQ];
  last = 0;
  first = 0;
  lengthQ = sizeQ;
}

fQ::~fQ() {  // initialization of Queue
  delete[] fQueue;
}

int fQ::push(fP pointerF) {  // push element from the queue
  if ((last + 1) % lengthQ == first) return 1;
  fQueue[last++] = pointerF;
  last = last % lengthQ;
  return 0;
}

int fQ::pull(void) {  // pull element from the queue
  if (last != first) {
    fQueue[first++]();
    first = first % lengthQ;
    return 0;
  } else return 1;
}

void fQ::waitOn(unsigned long delay) {
  delay += millis();
  while (millis() < delay) pull();
}

////NO RTOS END//////////
GyverOLED<SSH1106_128x64> oled;
class Kbd {
public:
  int pressed;
  int command;
  int scanline;
  Kbd();
};
Kbd::Kbd() {
  this->pressed = 0;
  this->command = -1;
  this->scanline = 0;
}

fQ funcQ(10);
fQ funcQ2(10);
Kbd kbd;

void dispClear(void) {
  oled.clear();
  oled.update();
}

void dispStr(void) {
  oled.clear();
  oled.update();  // SCREEN UPDATE
  oled.setScale(1);
  oled.setCursor(0, 0);
  oled.print("cmd");
 
  oled.setScale(4);
  oled.setCursor(30, 0);
  oled.print(dispArr);

  if(cmdSent){
      oled.setScale(1);
      oled.setCursor(10, 5);
      oled.print("track selected");
      oled.setScale(1);
      oled.setCursor(10, 7);
      if(cmdSent == 1) oled.print("Press # to PLAY");
      if(cmdSent == 2) oled.print("Press # to STOP");
  }
    oled.update();
  
}

void playStopCmd(void){ //play button manipulation
  if(cmdSent == 1) 
    {
        digitalWrite(13, HIGH);//press 0
        funcQ.waitOn(100);
        digitalWrite(13, LOW);
        cmdSent = 2;
    }
  else 
    {
        digitalWrite(13, HIGH);//press 0
        funcQ.waitOn(2000);
        digitalWrite(13, LOW);
        cmdSent = 1;
    }    
  
}

void pressTwoBtnFnc(int key_one, int key_two){
        digitalWrite(key_one, HIGH);//press 0
        funcQ.waitOn(100);
        digitalWrite(key_one, LOW); 
        funcQ.waitOn(100);
        digitalWrite(key_two, HIGH);
        funcQ.waitOn(100);
        digitalWrite(key_two, LOW);
        funcQ.waitOn(100);
}
 
void doCommand(void) { // track select
  int i = 2;
  int kbdNumb; //pressing number 
if (!cmdSent)  
  while (i) {
    kbdNumb = (kbd.command >> (4 * (i-1))) & 0x000F;   
    switch (kbdNumb) {
      case 0:
        pressTwoBtnFnc(2, 2);
        break;
      case 1:
        pressTwoBtnFnc(2, 5);
        break;
      case 2:
        pressTwoBtnFnc(5, 2);
        break;
      case 3:
        pressTwoBtnFnc(5, 5);
        break;
      case 4:
        pressTwoBtnFnc(5, 4);
        break;
      case 5:
        pressTwoBtnFnc(4, 5);
        break;
      case 6:
        pressTwoBtnFnc(4, 4);
        break;
      case 7:
        pressTwoBtnFnc(4, 3);
        break;
      case 8:
        pressTwoBtnFnc(3, 4);
        break;
      case 9:
        pressTwoBtnFnc(3, 3);
        break;
    }
    Serial.print(kbdNumb);  
    i--;
  }
  cmdSent = 1;
}

// scan keyboard
void scanColumn(int digStartNumb, int columnsN, int scanLine) { 
  for (int i = 0; i < columnsN; i++) {
    if (digitalRead(digStartNumb + i) == HIGH && kbd.pressed == 0) {
      kbd.pressed = scanLine + 3 * i;
      if (kbd.pressed != 12) cmdSent = 0; 
      if (kbd.pressed == 10) {
        funcQ2.push(dispClear);
        dispArr[0] = 0x00;
        dispArr[1] = 0x00;
      } else if (kbd.pressed == 11) {
        if (ind_dispArr == 0) dispArr[1] = 0x00;
        dispArr[ind_dispArr] = 0x30;
        ind_dispArr++;
        ind_dispArr %= DISPARRSTRLEN;
      } else if (kbd.pressed == 12) {
        if (dispArr[0] && dispArr[1]) {
          kbd.command = (((int)dispArr[0] - 0x0030)<<4 )+ (int)dispArr[1] - 0x0030;
        } 
        else{ 
        if (dispArr[0]){
           kbd.command = (int)dispArr[0] - 0x0030;
          }
        }          
        if (kbd.command != -1) {                // if some track select command is ready
          if(!cmdSent)funcQ2.push(doCommand);   // and not sent yet
          else funcQ2.push(playStopCmd);        // or sent already
        } 
      } else {
        if (ind_dispArr == 0) dispArr[1] = 0x00;
        dispArr[ind_dispArr] = kbd.pressed + 0x30;
        ind_dispArr++;
        ind_dispArr %= DISPARRSTRLEN;
      }
      funcQ2.push(dispStr);
    }
    if (digitalRead(digStartNumb + i) == LOW) {
      if (kbd.pressed == scanLine + 3 * i)
        kbd.pressed = 0;
      kbd.scanline = 0;
    }
  }
}

volatile int count = 0;

void keyboardScan(void) {
  if(kbd.scanline)
        scanColumn(6, 4, kbd.scanline);
  digitalWrite(10, LOW);
  digitalWrite(11, LOW);
  digitalWrite(12, LOW);
}


// Timer 1 interrupt service routine (ISR)
ISR(TIMER1_COMPA_vect) {
  // Sample voltage on pin A0
  digitalWrite(10 + count, HIGH);
  kbd.scanline = count + 1;
  funcQ.push(keyboardScan);
  count++;
  count %= 3;
}


void setup() {
  // put your setup code here, to run once:
  //CLKPR = 0x80; // (1000 0000) enable change in clock frequency
  //CLKPR = 0x01; // (0000 0001) use clock division factor 2 to reduce the frequency from 16 MHz to 8
  //Wire.begin();
  oled.init();
  dispStr();

  Serial.begin(9600);

  pinMode(2, OUTPUT);  //buttons
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(13, OUTPUT);

  digitalWrite(2, LOW);  //0 button (SW4)
  digitalWrite(3, LOW);  //9 button (SW1)
  digitalWrite(4, LOW);  //6 button (SW2)
  digitalWrite(5, LOW);  //3 button (SW3)
  digitalWrite(13, LOW);  //PLAY-STOP button (SW5)

  pinMode(10, OUTPUT);  //KBD scan 3 COLS
  pinMode(11, OUTPUT);
  pinMode(12, OUTPUT);

  pinMode(6, INPUT);  // kbd read 4 ROWS
  pinMode(7, INPUT);  //
  pinMode(8, INPUT);  //
  pinMode(9, INPUT);  //

  cli();  // disable interrupts during setup
  // Configure Timer 1 interrupt
  // F_clock = 16e6 Hz, prescaler = 64, Fs = 100 Hz
  TCCR1A = 0;
  TCCR1B = 1 << WGM12 | 0 << CS12 | 1 << CS11 | 1 << CS10;
  TCNT1 = 0;  // reset Timer 1 counter
  // OCR1A = ((F_clock / prescaler) / Fs) - 1 = 2499
  OCR1A = 2499;          // Set sampling frequency Fs = 100 Hz
  TIMSK1 = 1 << OCIE1A;  // Enable Timer 1 interrupt

  sei();  // re-enable interrupts
}

void loop() {
  // put your main code here, to run repeatedly:
  funcQ.pull();
  funcQ2.pull();
}
