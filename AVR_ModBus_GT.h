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
#define QUANTITY_REG_0X 5 //���������� ���������� ������� (DO) / r0x01, w0x05, w0x0F(15)
#define QUANTITY_REG_1X 2 //���������� ���������� ������ (DI) / r0x02
#define QUANTITY_REG_3X 6 //���������� ���������� ������ (AI) / r0x04
#define QUANTITY_REG_4X 5 //���������� ���������� ������� (AO) r0x03, w0x06, w0x10(16)

//������ ������� ����������� ������� � �������� 0.1�� ��� 8���
#define GLOBAL_TIME_START() TCCR0 |= (1<<CS01);\
							 TIMSK |= (1<<TOIE0);\
							 TCNT0 = 156
// CS02 CS01 CS00 clk/n
//     0       0       1   clk/1
//     0       1       0   clk/8
//     0       1       1   clk/64
//     1       0       0   clk/256
//     1       0       1   clk/1024

//��������� ���������� �� ������������ ������ ��������, ������ ��������
#define StartTrans() UCSRB |= (1<<TXEN); UCSRB |= (1<<UDRIE);//SetBit(UCSRB,TXEN); SetBit(UCSRB,UDRIE);
//��������� ���������� �� ������������ ������ ��������, ��������� ��������
#define StopTrans() UCSRB &= ~(1<<TXEN); UCSRB &= ~(1<<UDRIE);//ClrBit(UCSRB,TXEN); ClrBit(UCSRB,UDRIE);

//���������� �����
uint32_t G_TIME; 

//�������� (modbus)  
uint8_t RegNum0x[(QUANTITY_REG_0X+7)/8];//1-9999 Discrete Output Coils
uint8_t RegNum1x[(QUANTITY_REG_1X+7)/8];//10001-19999 Discrete Input Contacts
uint16_t RegNum3x[QUANTITY_REG_3X];//30001-39999 Analog Input Registers
uint16_t RegNum4x[QUANTITY_REG_4X];//40001-49999 Analog Output Holding Registers

void InitModBus(void);
void CheckModBus(void);
