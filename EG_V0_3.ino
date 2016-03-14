int8_t AddressReply=false;
int8_t Master=true;
//int8_t Master=false;
int8_t Address[4]={'1','2','3',0};
int8_t IsSelected=SELECT_STATE_FALSE;


extern int8_t KeyState,KeyValue;
int8_t CallState=CallState_NULL;

int8_t VoiceTXBuffer[VBUFF_SIZE]={0},VTXPointer=0;
int8_t VoiceRXBuffer[VBUFF_SIZE]={0},VRXPointer=0;
extern int8_t TXBuffer[];

boolean VoiceEnable=false;
boolean VoiceEnableOld=false;

boolean Div2,Div4;
boolean IsOpen=false;
int8_t ADCChannel=0;

Word ADC0,ADC1;

/* 
 16M / 16 / 13 = 76.9kHz
 the sampling frq shall between 50-200kHz to ensure the 10-bits precision
 ADPS[2 1 0] division  @16M
 0 0 0     2      615.4kHz
 0 0 1     2      615.4kHz
 0 1 0     4      307.7kHz
 0 1 1     8      153.8kHz
 1 0 0     16     76.9kHz
 1 0 1     32     38.5kHz
 1 1 0     64     19.2kHz
 1 1 1     128    9.6kHz
 */
void initADC(){
  sbi(ADCSRA, ADPS2);
  cbi(ADCSRA, ADPS1);
  cbi(ADCSRA, ADPS0);
  //right adjustment
  sbi(ADMUX,ADLAR);  
  //Reference=VCC
  sbi(ADMUX,REFS0);  
  cbi(ADMUX,REFS1);
  //enable adc
  sbi(ADCSRA,ADEN);
  // Set Input Multiplexer to Channel 0
  cbi(ADMUX,MUX0);   
  cbi(ADMUX,MUX1);
  cbi(ADMUX,MUX2);
  cbi(ADMUX,MUX3);
  //start the first sample
  sbi(ADCSRA,ADSC);
}

//Timer2 for ADC sampling & audio output
void initTimer2(){      
  // Timer2 PWM Mode set to fast PWM 
  sbi (TCCR2A, COM2B1);
  cbi (TCCR2A, COM2B0);
  cbi (TCCR2B, WGM22);
  sbi (TCCR2A, WGM21);
  sbi (TCCR2A, WGM20);
  /* Timer2 Clock Prescaler
   CS2[2 1 0] division   @16M
   0 0 0     no      stop
   0 0 1     1       62.5kHz
   0 1 0     8       7.8kHz
   0 1 1     32      1.95Hz
   1 0 0     64      976.6Hz
   1 0 1     128     488.3Hz
   1 1 0     256     244.1Hz
   1 1 1     1024    61Hz
   */
  cbi (TCCR2B, CS22);
  cbi (TCCR2B, CS21);
  sbi (TCCR2B, CS20);
  // Timer2 PWM Port Enable
  sbi(DDRD,3);                    // set digital pin3 to output
  sbi (TIMSK2,TOIE2);
}

void setup(){
  LcdInitialise();
  LcdClear();
  initADC();
  initTimer2();
  rs485_Init();
  pinMode(RELAY_PIN, OUTPUT);

  gotoXY(14,0);
  LcdString("Welcome");
  gotoXY(0,3);
  LcdString("  Entrance");
  gotoXY(3,4);
  LcdString("   Guard");
  gotoXY(0,5);
  LcdString("Lichao 2014");
  
}

int8_t number[4]={0,0,0,0},num_pointer=0;
void add_num(int8_t num){
  if(num>='0'&&num<='9')
    number[num_pointer++]=num;
  if(num_pointer>2)num_pointer=2;
}

void clear_num(){
  num_pointer=0;
  number[0]=0;
  number[1]=0;
  number[2]=0;
  number[3]=0;
}

void loop(){
  gotoXY(0,2);
  LcdString("Address:");
  LcdString(Address);
  gotoXY(0,1);
  LcdString("num=");
  LcdString(number);
  LcdString("     ");

  if(Master==true){
    IsSelected=SELECT_STATE_TRUE;
    Address[0]='0';
    Address[1]='0';
    Address[2]='0';
  }
    

  int8_t key=keyScan(ADC1.val>>6);
  switch (key) {
      case 'a':   VoiceEnable=false;             //call
                  delay(10);
                  calling();
                  VoiceEnable=false;
                  delay(10);
                  sendCommand(COMMAND_CALL_OFF);
                  gotoXY(0,1);
                  LcdString("            ");
                  break;
      case 'b':   break;                          //answer
      case 'c':   break;                          //off
      case 'y':   clear_num();break; 
      case 'n':   if(number[0]==0&&number[1]==0&&number[1]==0)
                    break;
                  Address[0]=number[0];
                  Address[1]=number[1];
                  Address[2]=number[2];
                  IsSelected=SELECT_STATE_FALSE;
                  clear_num();
                  break; 
      case 'd':   if(IsSelected==SELECT_STATE_TRUE){
                    VoiceEnableOld=VoiceEnable;  //open
                    VoiceEnable=false;
                    delay(100);
                    sendCommand(COMMAND_OPEN);
                    gotoXY(0,1);
                    LcdString("   open    ");
                    delay(1500);
                    gotoXY(0,1);
                    LcdString("            ");
                    VoiceEnable=VoiceEnableOld;
                  }
                  break;
      default:add_num(key);
        //save number
  }

  if(IsSelected==SELECT_STATE_CONFIRM){//ask reply
    VoiceEnable=false;
    delay(10);
    sendCommand(COMMAND_CONFIRM_Address);
    IsSelected=SELECT_STATE_TRUE;
  }
  if(IsSelected==SELECT_STATE_TRUE){
    if(IsOpen==true){
      digitalWrite(RELAY_PIN, HIGH);
      gotoXY(0,1);
      LcdString("open recieve");
      delay(1500);
      digitalWrite(RELAY_PIN, LOW);
      IsOpen=false;
      gotoXY(0,1);
      LcdString("            ");
    }
    
    if(CallState==CallState_WAITE){
      gotoXY(0,1);
      LcdString("visitor call");
      CallState=ringtone();
      if(CallState==CallState_ON){
        VoiceEnable=false;
        delay(10);
        sendCommand(COMMAND_CALL_BACK);
        VoiceEnable=true;
        while(1){         //in calling
              gotoXY(0,1);
              LcdString("     on     ");
              if(CallState==CallState_OFF)break;
              if(keyScan(ADC1.val>>6)=='c')break;
            }
        VoiceEnable=false;
        delay(10);
        sendCommand(COMMAND_CALL_OFF);
        gotoXY(0,1);
        LcdString("            ");
      }else if(CallState==CallState_OFF){
        VoiceEnable=false;
        delay(10);
        sendCommand(COMMAND_CALL_OFF);
        VoiceEnable=false;
        gotoXY(0,1);
        LcdString("    off     ");
        delay(1500);
        gotoXY(0,1);
        LcdString("            ");
      }
    CallState==CallState_NULL;
    }
  }
} // loop



ISR(TIMER2_OVF_vect) {
  Div2=!Div2;// divide timer2 frequency / 2 to 31.25kHz
  //div32=1;
  if (Div2){
    Div4=!Div4; // divide timer2 frequency / 2 to 15.6kHz
    if(Div4){
      if(ADCChannel==0){//voice sample,rate=7.8kHz
        ADC0.v[0]=ADCL;
        ADC0.v[1]=ADCH; 
        //OCR2B=ADC0.v[1];
        if(VoiceEnable)
          sendVoice(ADC0.v[1]);
        // Set Input Multiplexer to Channel 1
        ADCChannel=1;
        sbi(ADMUX,MUX0);   
        if(VRXPointer>0)//play the received voice
          OCR2B=VoiceRXBuffer[VRXPointer--]; 
      }
      else if(ADCChannel==1){//key sample,rate=7.8kHz
        ADC1.v[0]=ADCL;
        ADC1.v[1]=ADCH; 

        // Set Input Multiplexer to Channel 0
        ADCChannel=0;
        cbi(ADMUX,MUX0);   
      }
      sbi(ADCSRA,ADSC);
    }//Div4
  }//Div2
}
