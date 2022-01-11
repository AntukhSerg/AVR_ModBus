/*
* ��� ��� ������ �� ��������� modbus
* 
* void main( void )
* {
* InitModBus();
* sei(); //���������� ���������� ����������
* while(1)
*   {
*	CheckModBus();
*   } //end while(1)
* } //end main()
* 
* ���������� �������� ������������� ��������� InitModBus(), � ������������ ��������� 
* ������� CheckModBus() ��� �������� ������� ������� � ������������/�������� ������.
* ������ ������� �������� � ���������� ���������� RegNum0x[], RegNum1x[], RegNum3x[],
* RegNum4x[] � �������� �� ��� ��. ���������� RegNumYx[] ������������� ������� ���������:
* Yx
* 1 - 9999 - 	������-������ Discrete Output Coils (DO)
* 10001-19999 - ������	Discrete Input Contacts	(DI)
* 30001-39999 - ������	Analog Input Registers	(AI)
* 40001-49999 - ������-������	Analog Output Holding Registers	(AO)
* ���������� ��������� �������� � ����-������� ���������� QuantityRegYx, Y - 0,1,3,4
* c ���������� �������������� �� 0 �� 256.
*/

#define F_CPU 8000000L

//UART
#define BAUD_RATE 9600L
#define BAUD_DIVIDER (F_CPU/(16*BAUD_RATE)-1)

#define UBRRHi  UBRRH
#define UBRRLow UBRRL
#define UCSRA_CLR() UCSRA=0
#define UCSRB_SET() UCSRB=1<<RXEN|1<<TXEN|1<<RXCIE|0<<TXCIE
#define UCSRC_SET() UCSRC=1<<URSEL|1<<UCSZ0|1<<UCSZ1 //8bit, Even, 1stop.bit

//ModBus
//ID ������������
#define SLAVE_ID 0x0A
//������������ ������ ������ �����������(Rx)/������������(Tx) �� UART ������, ����.
#define MAX_LENGHT_REC_BUF 25
#define MAX_LENGHT_TR_BUF 25
//���������� ���������
#define QUANTITY_REG_0X 4 //���������� ���������� ������� (DO) / r0x01, w0x05, w0x0F(15)
#define QUANTITY_REG_1X 2 //���������� ���������� ������ (DI) / r0x02
#define QUANTITY_REG_3X 4 //���������� ���������� ������ (AI) / r0x04
#define QUANTITY_REG_4X 6 //���������� ���������� ������� (AO) r0x03, w0x06, w0x10(16)

//������ �������0 ��� �������� ����� 3,5�������
#define StartTimer0() TCCR0=1<<CS02;  TCNT0=130; 
// CS02 CS01 CS00 clk/n
//  0    0    1   clk/1
//  0    1    0   clk/8
//  0    1    1   clk/64
//  1    0    0   clk/256
//  1    0    1   clk/1024
//MaxPause t3_5= 3.5*(1+8+1+1)/baud  -> TCNT0 = 256-(clkTn*t3_5)
//��� clkTn = F_CPU / 1 (8, 64, 256, 1024)
// ���� baud >= 19200, �� t3_5 = 1.75ms
//TCNT0 = 200  ��� 8Mhz/256 baud >= 19200
//TCNT0 = 130  ��� 8Mhz/256 baud = 9600  

//��������� �������0
#define StopTimer0() TCCR0 = 0; TCNT0=0;
//��������� ���������� �� ������������ �������0
#define Timer0InterruptEnable() TIMSK=1<<TOIE0;
//��������� ���������� �� ������������ ������ ��������, ������ ��������
#define StartTrans() UCSRB |= (1<<TXEN); UCSRB |= (1<<UDRIE);//SetBit(UCSRB,TXEN); SetBit(UCSRB,UDRIE);
//��������� ���������� �� ������������ ������ ��������, ��������� ��������
#define StopTrans() UCSRB &= ~(1<<TXEN); UCSRB &= ~(1<<UDRIE);//ClrBit(UCSRB,TXEN); ClrBit(UCSRB,UDRIE);


//�������� (modbus)  
unsigned char RegNum0x[(QUANTITY_REG_0X+7)/8];//1-9999 Discrete Output Coils
unsigned char RegNum1x[(QUANTITY_REG_1X+7)/8];//10001-19999 Discrete Input Contacts
unsigned int RegNum3x[QUANTITY_REG_3X];//30001-39999 Analog Input Registers
unsigned int RegNum4x[QUANTITY_REG_4X];//40001-49999 Analog Output Holding Registers

void InitModBus(void);
void CheckModBus(void);
