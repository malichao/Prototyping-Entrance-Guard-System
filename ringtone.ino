extern void send_address(char data[]); 
char ringtone()
{
  gotoXY(0,3);
  LcdNum(2);
  unsigned long time=millis();
  char key;
  int i,j;
  while(millis()-time<30000)//30s
  {
    for(i=0;i<450;i++)
      {
        OCR2B=255;
        delayMicroseconds(500);
        OCR2B=0;
        delayMicroseconds(500);
        key=keyScan(adc1.val>>6);
        if(key=='b')
          return CALL_STATE_ON;
        else if(key=='c')
          return CALL_STATE_OFF;
        if(call_state==CALL_STATE_OFF)
          return CALL_STATE_OFF;
      }

      for(i=0;i<400;i++)
      {
        delayMicroseconds(500);
        delayMicroseconds(500);
        key=keyScan(adc1.val>>6);
        if(key=='b')
          return CALL_STATE_ON;
        else if(key=='c')
          return CALL_STATE_OFF;
        if(call_state==CALL_STATE_OFF)
          return CALL_STATE_OFF;
      }
    }  
  return CALL_STATE_OFF; 
}

void calling()
{
  unsigned long time=millis();
  char key;
  int i,j;
  address_reply=false;
  gotoXY(0,1);
  LcdString("            ");
  gotoXY(0,1);
  LcdString("call ");
  LcdString(number);
  voice_enable=false;
  if(num_pointer==0)
  {
    gotoXY(0,1);
    LcdString("            ");
    gotoXY(0,1);
    LcdString("empty addr");
    delay(1500);
    return;
  }
  while(millis()-time<1000)//1s
  {
    send_address(number); 
    delay(10);
    if(address_reply==true)break;
  }
  clear_num();
  if(address_reply==false)
  {
    gotoXY(0,1);
    LcdString("            ");
    gotoXY(0,1);
    LcdString("empty addr");
    delay(1500);
    return;
  }
  gotoXY(0,1);
  LcdString("  calling");
  send_command(COMMAND_CALL);
  while(millis()-time<30000)//30s
  {
    for(i=0;i<450;i++)
      {
        OCR2B=255;
        delayMicroseconds(500);
        OCR2B=0;
        delayMicroseconds(500);
        key=keyScan(adc1.val>>6);
        if(key=='c')
          return;
        if(call_state==CALL_STATE_ON)
          while(1)
          {
            gotoXY(0,1);
            LcdString("     on     ");
            if(call_state==CALL_STATE_OFF)return;
            if(keyScan(adc1.val>>6)=='c')return;
          }
      }

      for(i=0;i<400;i++)
      {
        delayMicroseconds(500);
        delayMicroseconds(500);
        key=keyScan(adc1.val>>6);
        if(key=='c')
          return;
        if(call_state==CALL_STATE_ON)
          while(1)
          {
             gotoXY(0,1);
             LcdString("     on     ");
             if(call_state==CALL_STATE_OFF)return;
             if(keyScan(adc1.val>>6)=='c')return;
          }
      }
    } 
  return; 
}