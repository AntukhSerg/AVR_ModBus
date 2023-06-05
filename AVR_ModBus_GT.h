/*
* Код для работы по протоколу modbus
* 
* void main( void )
* {
* InitModBus();
* sei(); //глобальное разрешение прерываний
* while(1)
*   {
*	CheckModBus();
*   } //end while(1)
* } //end main()
* 
* необходимо провести инициализацию протокола InitModBus(), и периодически запускать 
* функцию CheckModBus() для проверки наличия запроса и формирования/отправки ответа.
* данные заранее вносятся в глобальные переменные RegNum0x[], RegNum1x[], RegNum3x[],
* RegNum4x[] и беруться из них же. Переменные RegNumYx[] соответствуют номерам регистров:
* Yx
* 1 - 9999 - 	Чтение-запись Discrete Output Coils (DO)
* 10001-19999 - Чтение	Discrete Input Contacts	(DI)
* 30001-39999 - Чтение	Analog Input Registers	(AI)
* 40001-49999 - Чтение-запись	Analog Output Holding Registers	(AO)
* количество регистров задается в одно-байтной переменной QuantityRegYx, Y - 0,1,3,4
* c адресацией соответственно от 0 до 256.
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
//ID оборудования
#define SLAVE_ID 0x0A
//максимальный размер буфера принимаемых(Rx)/передаваемых(Tx) по UART данных, байт.
#define MAX_LENGHT_REC_BUF 25
#define MAX_LENGHT_TR_BUF 25
//количество регистров
#define QUANTITY_REG_0X 5 //количество дискретных выходов (DO) / r0x01, w0x05, w0x0F(15)
#define QUANTITY_REG_1X 2 //количество дискретных входов (DI) / r0x02
#define QUANTITY_REG_3X 6 //количество аналоговых входов (AI) / r0x04
#define QUANTITY_REG_4X 5 //количество аналоговых выходов (AO) r0x03, w0x06, w0x10(16)

//запуск таймера глобального вермени с периодом 0.1мс для 8МГц
#define GLOBAL_TIME_START() TCCR0 |= (1<<CS01);\
							 TIMSK |= (1<<TOIE0);\
							 TCNT0 = 156
// CS02 CS01 CS00 clk/n
//     0       0       1   clk/1
//     0       1       0   clk/8
//     0       1       1   clk/64
//     1       0       0   clk/256
//     1       0       1   clk/1024

//разрешить прерывание по освобождению буфера передачи, начать передачу
#define StartTrans() UCSRB |= (1<<TXEN); UCSRB |= (1<<UDRIE);//SetBit(UCSRB,TXEN); SetBit(UCSRB,UDRIE);
//запретить прерывание по освобождению буфера передачи, остановка передачи
#define StopTrans() UCSRB &= ~(1<<TXEN); UCSRB &= ~(1<<UDRIE);//ClrBit(UCSRB,TXEN); ClrBit(UCSRB,UDRIE);

//глобальное время
uint32_t G_TIME; 

//регистры (modbus)  
uint8_t RegNum0x[(QUANTITY_REG_0X+7)/8];//1-9999 Discrete Output Coils
uint8_t RegNum1x[(QUANTITY_REG_1X+7)/8];//10001-19999 Discrete Input Contacts
uint16_t RegNum3x[QUANTITY_REG_3X];//30001-39999 Analog Input Registers
uint16_t RegNum4x[QUANTITY_REG_4X];//40001-49999 Analog Output Holding Registers

void InitModBus(void);
void CheckModBus(void);
