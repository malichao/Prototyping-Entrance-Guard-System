char key_table[4][4]={
  '1','2','3','a',
  '4','5','6','b',
  '7','8','9','c',
  '0','y','n','d'};
#define delta_error 30//12
#define NO_KEY '!'
#define KEY_PRESS '('
#define KEY_HOLD '_'
#define KEY_UP ')'
#define KEY_PERIOD 20 //ms
#define KEY_CHECK '@'
char key_val=NO_KEY;

#define key_state0 0
#define key_state1 1
#define key_state2 2
char key_state=key_state0;
char keyScan(unsigned val)
{
  static unsigned long key_timer=0;
  char key_return=NO_KEY;
  if(millis()-key_timer<KEY_PERIOD)
    return NO_KEY;  
  
  key_timer=millis();  
  char key;
  if(val<25)
  {
    key=NO_KEY;
  }
  else
  {
    val+=delta_error;
    key=key_table[(16-(val)/59)/4][(16-(val)/59)%4];
  }
  switch(key_state)
  {
    case key_state0:
      if(key!=NO_KEY) key_state=key_state1;
      break;
    case key_state1:
      if(key!=NO_KEY)
      { 
        key_state=key_state2;
        key_return=key;
        key_val=key;
      }
      break;
    case key_state2:
      if(key==NO_KEY) key_state=key_state0;
      break;
  }
 return key_return;
}
