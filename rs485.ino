extern uint8_t crc_table[];

#define BUAD 512000//256000=31.25
#define TIMER1_TIME (65536-125)//62.5us
uint32_t tx_timer = 0;

#define TX_BUFF_SIZE 20
int8_t tx_Buff[TX_BUFF_SIZE] = { 0 };
int16_t tx_head = 0, tx_tail = 0;
#define TX_DELAY_TIME 100//at least 74us,for process the recieved data

#define RX_BUFF_SIZE 20
#define RECIEVE_EMPTY 0
#define RECIEVING 1
#define RECIEVE_FINISH 2
int8_t rx_recieve_flag = 0;
int8_t rx_Data = 0;
int8_t rx_Buff[RX_BUFF_SIZE] = { 0 };
int8_t *rx_p = rx_Buff, *rx_p_head = rx_Buff, *rx_p_end = &rx_Buff[9];
int8_t rx_pointer = 0;
boolean led;
#define LCD_BUFF_SIZE 10
unsigned LCD_Buff[LCD_BUFF_SIZE] = { 0 };

void rs485_Init() {

  // Set baud rate
  unsigned int16_t
  UBRR0_value;
  if (BUAD < 57600) {
    //UBRR0_value = ((F_CPU / (8L * baudrate)) - 1)/2 ;
    UBRR0_value = ((F_CPU / (8L * BUAD)) - 1) / 2;
    UCSR0A &= ~(1 << U2X0); // baud doubler off  - Only needed on Uno XXX
  } else {
    //UBRR0_value = ((F_CPU / (4L * baudrate)) - 1)/2;
    UBRR0_value = ((F_CPU / (4L * BUAD)) - 1) / 2;
    UCSR0A |= (1 << U2X0); // baud doubler on for high baud rates, i.e. 115200
  }
  UBRR0H = UBRR0_value >> 8;
  UBRR0L = UBRR0_value;

  // enable rx and tx
  UCSR0B |= 1 << RXEN0;
  UCSR0B |= 1 << TXEN0;

  int8_t t = UDR0;
  // enable interrupt on complete reception of a byte
  UCSR0B |= 1 << RXCIE0;

  // defaults to 8-bit, no parity, 1 stop bit
  timer1_Init();

}

void start_Timer1() {
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
void stop_Timer1() {
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

void timer1_Init() {
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
int8_t rs485_tx_available() {
  if (tx_tail == 0)
    if (micros() - tx_timer > TX_DELAY_TIME)
      return 1;
  return 0;
}

ISR(USART_UDRE_vect)
{
  UDR0 = tx_Buff[tx_head++];
  if(tx_head==tx_tail)
  {
    UCSR0B &= ~(1 << UDRIE0);
    tx_tail=0;
    tx_head=0;
    tx_timer=micros();
  }
}
void rs485_write(int8_t data) {
  tx_Buff[0] = data;
  tx_Buff[1] = crc_table[data];
  tx_tail = 2;
  tx_head = 0;
  // Enable Data Register Empty Interrupt to make sure tx-streaming is running
  UCSR0B |= (1 << UDRIE0);
}

void send_voice(int8_t data) {
  int8_t arr[2] = { FUNCTION_VOICE };
  arr[1] = data;
  int8_t crc = CRC8_Tables(arr, 2);
  int8_t i;
  for (i = 0; i < 2; i++)
    tx_Buff[i] = arr[i];
  tx_Buff[i++] = crc;
  tx_tail = i;
  tx_head = 0;
  // Enable Data Register Empty Interrupt to make sure tx-streaming is running
  UCSR0B |= (1 << UDRIE0);
}

void send_command(int8_t data) {
  int8_t arr[2] = { FUNCTION_COMMAND };
  arr[1] = data;
  int8_t crc = CRC8_Tables(arr, 2);
  int8_t i;
  for (i = 0; i < 2; i++)
    tx_Buff[i] = arr[i];
  tx_Buff[i++] = crc;
  tx_tail = i;
  tx_head = 0;
  // Enable Data Register Empty Interrupt to make sure tx-streaming is running
  UCSR0B |= (1 << UDRIE0);
}

void send_address(int8_t data[]) {
  int8_t arr[5] = { FUNCTION_COMMAND };
  arr[1] = COMMAND_CHECK_ADDRESS;
  arr[2] = data[0];
  arr[3] = data[1];
  arr[4] = data[2];
  int8_t crc = CRC8_Tables(arr, 2);
  int8_t i;
  for (i = 0; i < 5; i++)
    tx_Buff[i] = arr[i];
  tx_Buff[i++] = crc;
  tx_tail = i;
  tx_head = 0;
  // Enable Data Register Empty Interrupt to make sure tx-streaming is running
  UCSR0B |= (1 << UDRIE0);
}

void rs485_writes(int8_t *data, int8_t len) {
  int8_t crc = CRC8_Tables(data, len);
  int8_t i;
  for (i = 0; i < len; i++)
    tx_Buff[i] = *data++;
  tx_Buff[i++] = crc;
  tx_tail = i;
  tx_head = 0;
  // Enable Data Register Empty Interrupt to make sure tx-streaming is running
  UCSR0B |= (1 << UDRIE0);
}

ISR(TIMER1_OVF_vect){  //need 74us to process the recieved data
  stop_Timer1();
  rx_pointer=0;
  if(CRC8_Tables(rx_Buff,rx_pointer)==0){
    int16_t i;
    //rx_val+=rx_Buff[0];
    if(rx_Buff[0]==FUNCTION_VOICE){
      //state_flag=1;
      //if(voice_enable)
      OCR2B=rx_Buff[1];
    }
    else if(rx_Buff[0]==FUNCTION_COMMAND){
      switch (rx_Buff[1]) {
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
        if(rx_Buff[2]==address[0]
            &&rx_Buff[3]==address[1]
            &&rx_Buff[4]==address[2])
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
    rx_counter++;
  }
  Timer1_counter++;
}

ISR(USART_RX_vect){    ///must read the UDR0 to clear the RX interrupt flag!!
  rx_vect_counter++;
  rx_Buff[rx_pointer++]=UDR0;
  if(rx_pointer==RX_BUFF_SIZE)
  rx_pointer=0;
  start_Timer1();
}

