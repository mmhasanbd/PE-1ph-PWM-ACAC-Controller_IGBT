#include<avr/io.h>
#include<avr/interrupt.h>
#include<util/delay.h>
#include <TimerOne.h>
#define ZERO_CROSS_PIN    2
#define GATE_PIN_1       9     // +VE
#define GATE_PIN_2       10     // -VE

#define POT_PIN           A0
#define BUTTON_MID        7   // mode

#define maxPulseTime      1023
#define minPulseTime      0

volatile bool CycloCovnStat;
volatile unsigned int timer_pulse_time = 0,dutyCycle;
float req_pulse_time= 1, count=55; 

void setup() {
  Timer1.initialize(400);  // 100uS
  Timer1.pwm(GATE_PIN_1, 0);
  Timer1.pwm(GATE_PIN_2, 0);
  DDRB = 0xFF;
  EIMSK |= (1 << INT0) ;                          // External Interrupt Enable
  EICRA |= 0x01 ;                                 // Interrupt at Logic Chang
}

void checkButton(){
  if(digitalRead(BUTTON_MID)){
    delay(300);
    CycloCovnStat = !CycloCovnStat;
  }
}

void loop() {
  checkButton();
  pulseDuration();
  delay(50);                        // loop delay
}

void pulseDuration(){
  unsigned long _adc_sum = 0;
  int _sum_count = 0;
  while(_sum_count < 10){
    _adc_sum += analogRead(POT_PIN);
    _sum_count ++;
    delayMicroseconds(100);
  }
  float _adc_avg = float(_adc_sum)/float(_sum_count);
  dutyCycle= map(_adc_avg,0,1023,minPulseTime,maxPulseTime);
}

void CycloCovnPulseOff(void){
  PORTB = 0x00;
  TCCR2B = 0;                                     // stop timer two
  TIMSK2 &= ~(1 << TOIE2);                        // Timer2 Interrupt Disable
  Timer1.setPwmDuty(GATE_PIN_1,0);
  Timer1.setPwmDuty(GATE_PIN_2,0);
}

void Timer2Enable(void){
  TCNT2 = 0xDC;                                   // 220 -> 120 us delay
  TCCR2B = (1 << CS21)  ;                         // clkI/O/8 (From prescaler) At 16MHz each (8bit=256)count 0.128 ms
  TIFR2 |= (1 << TOV2)  ;
  TIMSK2 |= (1 << TOIE2);                         // Timer2 Interrupt Enable
}



ISR(INT0_vect){                                   // External Interrupt Service Routine
  if(CycloCovnStat){
    timer_pulse_time = 0;
    CycloCovnPulseOff();
    Timer2Enable();                                 // Timer0 Enable
  
  }
}

ISR(TIMER2_OVF_vect){    // Timer0 Interrupt Service Routine each interrupt at 0.128 ms
  if(timer_pulse_time == req_pulse_time){ 
     Timer1.setPwmDuty(GATE_PIN_1, dutyCycle);
     Timer1.setPwmDuty(GATE_PIN_2, dutyCycle);
    }
   else if(timer_pulse_time >=count){
        Timer1.setPwmDuty(GATE_PIN_1,0);
        Timer1.setPwmDuty(GATE_PIN_2,0);
  }
  timer_pulse_time ++;                           // Increment Timer Overflow count
}
