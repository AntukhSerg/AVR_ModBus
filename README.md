# AVR_ModBus
Tested(ATMega8)

RU:   
Модуль для AVR. Реализация протокола MODBUS RTU. Функции 0х01 - 0х06.

Код для работы по протоколу modbus

```
void main( void )
{
InitModBus();
sei(); //глобальное разрешение прерываний
while(1)
  {
  CheckModBus();
  } //end while(1)
} //end main()
```

необходимо провести инициализацию протокола InitModBus(), и периодически запускать 
функцию CheckModBus() для проверки наличия запроса и формирования/отправки ответа.
данные заранее вносятся в глобальные переменные RegNum0x[], RegNum1x[], RegNum3x[],
RegNum4x[] и беруться из них же. 
* Переменные RegNumYx[] соответствуют номерам регистров:
* 0x - 1 - 9999 - 	Чтение-запись Discrete Output Coils (DO)
* 1x - 10001-19999 - Чтение	Discrete Input Contacts	(DI)
* 3x - 30001-39999 - Чтение	Analog Input Registers	(AI)
* 4x - 40001-49999 - Чтение-запись	Analog Output Holding Registers	(AO)

количество регистров задается в одно-байтной переменной QuantityRegYx (Y - 0,1,3,4)
c адресацией соответственно от 0 до 256.


EN:   
Module for AVR. Implementation of the MODBUS RTU protocol. Function Code 0x01 - 0x06.
Quoting code
Use Modbus
```
void main (void)
{
InitModBus();
sei(); // global interrupt enable
while (1)
  {
  CheckModBus();
  } // end while (1)
} // end main ()
```
it is necessary to initialize the InitModBus () protocol, and periodically run
the CheckModBus () function to check for a request and form / send a response.
data is entered in advance into global variables RegNum0x [], RegNum1x [], RegNum3x [], RegNum4x [] and taken from them. 
* Variables RegNumYx [] correspond to register numbers:
* 0x - 1 - 9999 - Read / Write Discrete Output Coils (DO)
* 1x - 10001-19999 - Reading Discrete Input Contacts (DI)
* 3x - 30001-39999 - Reading Analog Input Registers (AI)
* 4x - 40001-49999 - Read-Write Analog Output Holding Registers (AO)

the number of registers is set in a one-byte variable QuantityRegYx (Y - 0,1,3,4)
with addressing, respectively, from 0 to 256.
