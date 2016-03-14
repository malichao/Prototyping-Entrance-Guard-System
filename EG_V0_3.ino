int8_t address_reply=false;
int8_t master=true;
//int8_t master=false;
int8_t address[4]={'1','2','3',0};
int8_t selected=SELECT_STATE_FALSE;


extern int8_t key_state,key_val;
int8_t call_state=CALL_STATE_NULL;

int8_t voice_TBuff[VBUFF_SIZE]={0},vt_pointer=0;
int8_t voice_RBuff[VBUFF_SIZE]={0},vr_pointer=0;
extern int8_t tx_Buff[];

boolean voice_enable=false;
boolean voice_enable_old=false;

boolean div2,div4;
int8_t open_flag=false;
int8_t ring_flag=false;
int8_t adc_channel=0;

Word adc0,adc1;

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

uint32_t time,time1,time2,timeTx=0;
uint32_t rx_counter=0,rx_val=0,Timer1_counter=0,rx_vect_counter=0,rx_check_counter=0;
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

int8_t state,last_state,function;
#define CALL 0
#define OPEN 1
#define ADDRESS 2
#define Null 4
uint32_t open_millis=0;
void loop(){
  gotoXY(0,2);
  LcdString("address:");
  LcdString(address);
  gotoXY(0,1);
  LcdString("num=");
  LcdString(number);
  LcdString("     ");

  if(master==true){
    selected=SELECT_STATE_TRUE;
    address[0]='0';
    address[1]='0';
    address[2]='0';
  }
    

  int8_t key=keyScan(adc1.val>>6);
  switch (key) {
      case 'a':   voice_enable=false;             //call
                  delay(10);
                  calling();
                  voice_enable=false;
                  delay(10);
                  send_command(COMMAND_CALL_OFF);
                  gotoXY(0,1);
                  LcdString("            ");
                  break;
      case 'b':   break;                          //answer
      case 'c':   break;                          //off
      case 'y':   clear_num();break; 
      case 'n':   if(number[0]==0&&number[1]==0&&number[1]==0)
                    break;
                  address[0]=number[0];
                  address[1]=number[1];
                  address[2]=number[2];
                  selected=SELECT_STATE_FALSE;
                  clear_num();
                  break; 
      case 'd':   if(selected==SELECT_STATE_TRUE)
                  {
                    voice_enable_old=voice_enable;  //open
                    voice_enable=false;
                    delay(100);
                    send_command(COMMAND_OPEN);
                    gotoXY(0,1);
                    LcdString("   open    ");
                    delay(1500);
                    gotoXY(0,1);
                    LcdString("            ");
                    voice_enable=voice_enable_old;
                  }
                  break;
      default:add_num(key);
        //save number
  }

  if(selected==SELECT_STATE_CONFIRM){//ask reply
    voice_enable=false;
    delay(10);
    send_command(COMMAND_CONFIRM_ADDRESS);
    selected=SELECT_STATE_TRUE;
  }
  if(selected==SELECT_STATE_TRUE){
    if(open_flag==true){
      digitalWrite(RELAY_PIN, HIGH);
      gotoXY(0,1);
      LcdString("open recieve");
      delay(1500);
      digitalWrite(RELAY_PIN, LOW);
      open_flag=false;
      gotoXY(0,1);
      LcdString("            ");
    }
    
    if(call_state==CALL_STATE_WAITE){
      gotoXY(0,1);
      LcdString("visitor call");
      call_state=ringtone();
      if(call_state==CALL_STATE_ON){
        voice_enable=false;
        delay(10);
        send_command(COMMAND_CALL_BACK);
        voice_enable=true;
        while(1){         //in calling
              gotoXY(0,1);
              LcdString("     on     ");
              if(call_state==CALL_STATE_OFF)break;
              if(keyScan(adc1.val>>6)=='c')break;
            }
        voice_enable=false;
        delay(10);
        send_command(COMMAND_CALL_OFF);
        gotoXY(0,1);
        LcdString("            ");
      }else if(call_state==CALL_STATE_OFF){
        voice_enable=false;
        delay(10);
        send_command(COMMAND_CALL_OFF);
        voice_enable=false;
        gotoXY(0,1);
        LcdString("    off     ");
        delay(1500);
        gotoXY(0,1);
        LcdString("            ");
      }
    call_state==CALL_STATE_NULL;
    }
  }
} // loop



ISR(TIMER2_OVF_vect) {
  div2=!div2;// divide timer2 frequency / 2 to 31.25kHz
  //div32=1;
  if (div2){
    div4=!div4; // divide timer2 frequency / 2 to 15.6kHz
    if(div4){
      if(adc_channel==0){//voice sample,rate=7.8kHz
        adc0.v[0]=ADCL;
        adc0.v[1]=ADCH; 
        //OCR2B=adc0.v[1];
        if(voice_enable)
          send_voice(adc0.v[1]);
        // Set Input Multiplexer to Channel 1
        adc_channel=1;
        sbi(ADMUX,MUX0);   
        if(vr_pointer>0)//play the received voice
          OCR2B=voice_RBuff[vr_pointer--]; 
      }
      else if(adc_channel==1){//key sample,rate=7.8kHz
        adc1.v[0]=ADCL;
        adc1.v[1]=ADCH; 

        // Set Input Multiplexer to Channel 0
        adc_channel=0;
        cbi(ADMUX,MUX0);   
      }
      sbi(ADCSRA,ADSC);
    }//div4
  }//div2
}
