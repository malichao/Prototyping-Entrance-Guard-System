#define BUAD 512000//256000=31.25
#define TIMER1_TIME (65536-125)//62.5us
#define TX_BUFFER_SIZE 20
#define LCD_BUFF_SIZE 10
#define TX_DELAY_TIME 100         //at least 74us,for process the recieved data

#define RX_BUFFER_SIZE 20
#define RECIEVE_EMPTY 0
#define RECIEVING 1
#define RECIEVE_FINISH 2

extern uint8_t CRC8Table[];

uint32_t  txTimer = 0;
int8_t    txBuffer[TX_BUFFER_SIZE] = { 0 };
int16_t   txHead = 0, txTail = 0;

int8_t rxRecievedFlag = 0;
int8_t rxData = 0;
int8_t rxBuffer[RX_BUFFER_SIZE] = { 0 };
int8_t rxPointer = 0;
boolean led;

unsigned LCD_Buff[LCD_BUFF_SIZE] = { 0 };

void initRS485() {

  // Set baud rate
  unsigned int16_t UBRR0Value;
  if (BUAD < 57600) {
    //UBRR0Value = ((F_CPU / (8L * baudrate)) - 1)/2 ;
    UBRR0Value = ((F_CPU / (8L * BUAD)) - 1) / 2;
    UCSR0A &= ~(1 << U2X0); // baud doubler off  - Only needed on Uno XXX
  } else {
    //UBRR0Value = ((F_CPU / (4L * baudrate)) - 1)/2;
    UBRR0Value = ((F_CPU / (4L * BUAD)) - 1) / 2;
    UCSR0A |= (1 << U2X0); // baud doubler on for high baud rates, i.e. 115200
  }
  UBRR0H = UBRR0Value >> 8;
  UBRR0L = UBRR0Value;

  // enable rx and tx
  UCSR0B |= 1 << RXEN0;
  UCSR0B |= 1 << TXEN0;

  // enable interrupt on complete reception of a byte
  UCSR0B |= 1 << RXCIE0;

  // defaults to 8-bit, no parity, 1 stop bit
  timer1_Init();

}

void startTimer1() {
  /*Timer1 Clock Prescaler
   CS2[2 1 0] division   @16M
   0 0 0     no      stop
   0 0 1     1       16M        
   0 1 0     8       2M
   0 1 1     64      250kHz
   1 0 0     256     62.5kHz
   1 0 1     1024    15.6kHz
   1 1 0     Ext1  falling edge
   1 1 1     Ext1  rising edge
   */
  cli();
  TCNT1H = TIMER1_TIME >> 8;
  TCNT1L = TIMER1_TIME & 255;
  sei();
  cbi(TCCR1B, CS12);
  sbi(TCCR1B, CS11);
  cbi(TCCR1B, CS10);

}
void stopTimer1() {
  /*Timer1 Clock Prescaler
   CS2[2 1 0] division   @16M
   0 0 0     no      stop
   0 0 1     1       16M        
   0 1 0     8       2M
   0 1 1     64      250kHz
   1 0 0     256     62.5kHz
   1 0 1     1024    15.6kHz
   1 1 0     Ext1  falling edge
   1 1 1     Ext1  rising edge
   */
  cbi(TCCR1B, CS12);
  cbi(TCCR1B, CS11);
  cbi(TCCR1B, CS10);

}

void initTimer1() {
  //Normal mode
  cbi(TCCR1B, WGM13);
  cbi(TCCR1B, WGM12);
  cbi(TCCR1A, WGM11);
  cbi(TCCR1A, WGM10);
  //stop timer1 clock source
  cbi(TCCR1B, CS12);
  cbi(TCCR1B, CS11);
  cbi(TCCR1B, CS10);
  //clear the existed interrupt flag!!
  sbi(TIFR1, TOV1);
  //enanble timer1 overflow interrupt
  sbi(TIMSK1, TOIE1);
}
int8_t rs485TxAvailable() {
  if (txTail == 0)
    if (micros() - txTimer > TX_DELAY_TIME)
      return 1;
  return 0;
}

ISR(USART_UDRE_vect){
  UDR0 = txBuffer[txHead++];
  if(txHead==txTail){
    UCSR0B &= ~(1 << UDRIE0);
    txTail=0;
    txHead=0;
    txTimer=micros();
  }
}
void writeRS485(int8_t data) {
  txBuffer[0] = data;
  txBuffer[1] = crc_table[data];
  txTail = 2;
  txHead = 0;
  // Enable Data Register Empty Interrupt to make sure tx-streaming is running
  UCSR0B |= (1 << UDRIE0);
}

void sendVoice(int8_t data) {
  int8_t arr[2] = { FUNCTION_VOICE };
  arr[1] = data;
  int8_t crc = CRC8_Tables(arr, 2);
  int8_t i;
  for (i = 0; i < 2; i++)
    txBuffer[i] = arr[i];
  txBuffer[i++] = crc;
  txTail = i;
  txHead = 0;
  // Enable Data Register Empty Interrupt to make sure tx-streaming is running
  UCSR0B |= (1 << UDRIE0);
}

void sendCommand(int8_t data) {
  int8_t arr[2] = { FUNCTION_COMMAND };
  arr[1] = data;
  int8_t crc = CRC8_Tables(arr, 2);
  int8_t i;
  for (i = 0; i < 2; i++)
    txBuffer[i] = arr[i];
  txBuffer[i++] = crc;
  txTail = i;
  txHead = 0;
  // Enable Data Register Empty Interrupt to make sure tx-streaming is running
  UCSR0B |= (1 << UDRIE0);
}

void sendAddress(int8_t data[]) {
  int8_t arr[5] = { FUNCTION_COMMAND };
  arr[1] = COMMAND_CHECK_ADDRESS;
  arr[2] = data[0];
  arr[3] = data[1];
  arr[4] = data[2];
  int8_t crc = CRC8_Tables(arr, 2);
  int8_t i;
  for (i = 0; i < 5; i++)
    txBuffer[i] = arr[i];
  txBuffer[i++] = crc;
  txTail = i;
  txHead = 0;
  // Enable Data Register Empty Interrupt to make sure tx-streaming is running
  UCSR0B |= (1 << UDRIE0);
}

void writeBufferRS485(int8_t *data, int8_t len) {
  int8_t crc = CRC8_Tables(data, len);
  int8_t i;
  for (i = 0; i < len; i++)
    txBuffer[i] = *data++;
  txBuffer[i++] = crc;
  txTail = i;
  txHead = 0;
  // Enable Data Register Empty Interrupt to make sure tx-streaming is running
  UCSR0B |= (1 << UDRIE0);
}

ISR(TIMER1_OVF_vect){  //need 74us to process the recieved data
  stop_Timer1();
  rxPointer=0;
  if(CRC8_Tables(rxBuffer,rxPointer)==0){
    int16_t i;
    //rx_val+=rxBuffer[0];
    if(rxBuffer[0]==FUNCTION_VOICE){
      //state_flag=1;
      //if(voice_enable)
      OCR2B=rxBuffer[1];
    }
    else if(rxBuffer[0]==FUNCTION_COMMAND){
      switch (rxBuffer[1]) {
        case COMMAND_OPEN:
        open_flag=true;
        break;
        case COMMAND_CALL:
        call_state=CALL_STATE_WAITE;
        break;
        case COMMAND_CALL_BACK:
        voice_enable=true;
        call_state=CALL_STATE_ON;
        break;
        case COMMAND_CALL_OFF:
        call_state=CALL_STATE_OFF;
        break;
        case COMMAND_CHECK_ADDRESS:
        voice_enable=false;
        if(rxBuffer[2]==address[0]
            &&rxBuffer[3]==address[1]
            &&rxBuffer[4]==address[2])
        selected=SELECT_STATE_CONFIRM;
        else if(master==false)
        selected=SELECT_STATE_FALSE;
        break;
        case COMMAND_CONFIRM_ADDRESS:
        address_reply=true;
        break;
        default:break;
      }
    }
  }
}

ISR(USART_RX_vect){    ///must read the UDR0 to clear the RX interrupt flag!!
  rxBuffer[rxPointer++]=UDR0;
  if(rxPointer==RX_BUFFER_SIZE)
  rxPointer=0;
  startTimer1();
}

