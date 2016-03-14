#define delta_error 30//12
#define NO_KEY '!'
#define KEY_PRESS '('
#define KEY_HOLD '_'
#define KEY_UP ')'
#define KEY_PERIOD 20 //ms
#define KEY_CHECK '@'

#define KEY_STATE_0 0
#define KEY_STATE_1 1
#define KEY_STATE_2 2

int8_t keyValue = NO_KEY;
int8_t keyState = KEY_STATE_0;
int8_t keyTable[4][4]={
  '1','2','3','a',
  '4','5','6','b',
  '7','8','9','c',
  '0','y','n','d'};

int8_t keyScan(uint8_t val) {
  static uint32_t keyTimer = 0;
  int8_t keyReturnValue = NO_KEY;
  if (millis() - keyTimer < KEY_PERIOD)
    return NO_KEY;

  keyTimer = millis();
  int8_t key;
  if (val < 25) {
    key = NO_KEY;
  } else {
    val += delta_error;
    key = keyTable[(16 - (val) / 59) / 4][(16 - (val) / 59) % 4];
  }
  switch (keyState) {
  case KEY_STATE_0:
    if (key != NO_KEY)
      keyState = KEY_STATE_1;
    break;
  case KEY_STATE_1:
    if (key != NO_KEY) {
      keyState = KEY_STATE_2;
      keyReturnValue = key;
      keyValue = key;
    }
    break;
  case KEY_STATE_2:
    if (key == NO_KEY)
      keyState = KEY_STATE_0;
    break;
  }
  return keyReturnValue;
}

