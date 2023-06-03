//ATmega8, ATmega16
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdbool.h>
#include <util/crc16.h>
#include "AVR_ModBus.h"

#define Hi(Int) (char) (Int>>8)
#define Low(Int) (char) (Int)

unsigned char RcCount, TrCount;  //счетчик принятых/переданных данных
bool StartRec = false;// false/true начало/прием посылки
bool bModBus = false;  //флаг обработки посылки
unsigned char cNumRcByte0; //кол-во принятых байт
unsigned char cNumTrByte0;  //кол-во передаваемых байт
unsigned char cmRcBuf0[MAX_LENGHT_REC_BUF]; //буфер принимаемых данных
unsigned char cmTrBuf0[MAX_LENGHT_TR_BUF]; //буфер передаваемых данных

unsigned char ModBus(unsigned char NumByte);
char Func01(void);
char Func02(void);
char Func03(void);
char Func04(void);
char Func05(void);
char Func06(void);
char Func16(void);
char ErrorMessage(char Error);
unsigned int GetCRC16(unsigned char* buf, unsigned char bufsize);



//настройка UART
void InitModBus(void)
{
	UBRRHi = Hi(BAUD_DIVIDER);
	UBRRLow = Low(BAUD_DIVIDER);
	UCSRA_CLR();
	UCSRB_SET();
	UCSRC_SET();
	//останавливаем таймер0
	StopTimer0();
	//разрешаем прерывание по переполнению таймера0
	Timer0InterruptEnable();
}//end InitModBus()


void CheckModBus(void)
{
	if (bModBus)
	{
		cNumTrByte0 = ModBus(cNumRcByte0); //обработка принятого соообщения ModBus
		if (cNumTrByte0 != 0)
		{
			TrCount = 0;
			StartTrans();
		} //end if (cNumTrByte0!=0)
		bModBus = false;
	} //end if (bModBus)
} //end CheckModBus(void)




unsigned char ModBus(unsigned char NumByte)
{
	int CRC16;
	unsigned char i;

	if (cmRcBuf0[0] != SLAVE_ID) return 0x00; //broadcast запрос, ответ не нужен

	CRC16 = GetCRC16(cmRcBuf0, NumByte);
	if (CRC16) return 0;  //контрольная сумма не совпадает, ответ не нужен

	//проверки прошли успешно начинаем формировать ответ на запрос
	//данные для ответа заносятся в массив cmTrBuf0[]
	//!!!!! адреса и данные двубайтные заносятся старшим байтом вперед (Hi, Low)
	//!!!!! CRC заноситься младшим байтом вперед (Low, Hi) 

	cmTrBuf0[0] = SLAVE_ID;//адрес устройства

	for (i = 1; i < (MAX_LENGHT_TR_BUF); i++) //очищаем буфер для данных
	{
		cmTrBuf0[i] = 0;
	}//end for()

  //код команды
	switch (cmRcBuf0[1])
	{
	case 0x01:
	{
		if (!QUANTITY_REG_0X) return ErrorMessage(ERR_ILLEGAL_FUNCTION);
		return Func01();
	}

	case 0x02:
	{
		if (!QUANTITY_REG_1X) return ErrorMessage(ERR_ILLEGAL_FUNCTION);
		return Func02();
	}

	case 0x03:
	{
		if (!QUANTITY_REG_4X) return ErrorMessage(ERR_ILLEGAL_FUNCTION);
		return Func03();
	}

	case 0x04:
	{
		if (!QUANTITY_REG_3X) return ErrorMessage(ERR_ILLEGAL_FUNCTION);
		return Func04();
	}

	case 0x05:
	{
		if (!QUANTITY_REG_0X) return ErrorMessage(ERR_ILLEGAL_FUNCTION);
		return Func05();
	}

	case 0x06:
	{
		if (!QUANTITY_REG_4X) return ErrorMessage(ERR_ILLEGAL_FUNCTION);
		return Func06();
	}

	case 0x10:
	{
		if (!QUANTITY_REG_4X) return ErrorMessage(ERR_ILLEGAL_FUNCTION);
		return Func16();
	}

	default:   return ErrorMessage(ERR_ILLEGAL_FUNCTION); //Принятый код функции не может быть обработан
	} //end switch
} //end ModBus()



//функция вычисляет код CRC-16
//на входе указатель на начало буфера
//и количество байт сообщения (без принятого кода CRC-16)
unsigned int GetCRC16(unsigned char* buf, unsigned char bufsize)
{
	unsigned int crc = 0xFFFF;
	unsigned char i;

	for (i = 0; i < bufsize; i++)
		crc = _crc16_update(crc, buf[i]);//подсчет CRC в принятой посылке
	return crc;
} //end GetCRC16()



//формирование ответа об ошибке
char ErrorMessage(char Error)
{
	int CRC16;

	cmTrBuf0[1] = cmRcBuf0[1] + 0x80;//команда с ошибкой
	cmTrBuf0[2] = Error;
	CRC16 = GetCRC16(cmTrBuf0, 3);//подсчет КС посылки
	cmTrBuf0[3] = Low(CRC16);
	cmTrBuf0[4] = Hi(CRC16);

	return 5;
} //end ErrorMessage()



//---------------------------Функции ModBus-------------------------------

#define start_adress ((cmRcBuf0[2]<<8) + cmRcBuf0[3])
#define quantity_registers ((cmRcBuf0[4]<<8) + cmRcBuf0[5])
#define data_byte ((quantity_registers+7)/8)
#define data_2byte (2*quantity_registers) //данные двухбайтные
#define max_adress (start_adress + quantity_registers)
#define reg_adress (start_adress)
#define value (quantity_registers)
#define num_to_write (cmRcBuf0[6]) // количество байт данных для записи в функции 0x10
#define data_pointer (cmRcBuf0 + 7) // указатель на данные для записи в регистры
//побитовая запись
#define I_byte (reg_adress/8)
#define I_bit (reg_adress%8)
//побитовая запись с учетом смещения адреса
#define I_byte_ofset(x) ((start_adress+x)/8)
#define I_bit_ofset(x) ((start_adress+x)%8)
#define NextBit(y,x) RegNum##y[I_byte_ofset(x)] & (1<<I_bit_ofset(x)) 
#define ClrBitOfByte(ByteA, x) (ByteA &= ~(1<<(x)))


/*
* Проверяет входные данные на соответствие Вашим регистрам
* Возвращает код ошибки:
* 0 - данные валидны
* 2 - ILLEGAL_DATA_ADDRESS
* 3 - ILLEGAL_DATA_VALUE
*/
uint8_t check_request(int reg, int num)
{
	if (reg < 0 || reg > QUANTITY_REG_4X)
		return ERR_ILLEGAL_DATA_ADDRESS; //Адрес данных, указанный в запросе, недоступен
	if ((reg + num) > QUANTITY_REG_4X)
		return ERR_ILLEGAL_DATA_VALUE;  // Количество регистров выходит за реальное количество регистров
	return ERR_NONE;
}



//реализация функции 0х01
//чтение значений из нескольких регистров флагов (Read Coil Status)
char Func01(void)
{
	unsigned int CRC16;
	unsigned char i;

	//проверка корректного адреса в запросе и количество регистров на чтение
	char err = check_request(start_adress, quantity_registers);
	if (err != ERR_NONE)
		return ErrorMessage(err);

	//формируем ответ
	cmTrBuf0[1] = 0x01;//команда
	cmTrBuf0[2] = data_byte;//количество байт данных в ответе

	for (i = 0; i < quantity_registers; i++)
	{
		cmTrBuf0[3 + i / 8] |= ((bool)(NextBit(0x, i)) << (i % 8)); //данные
	}//end for()
	CRC16 = GetCRC16(cmTrBuf0, data_byte + 3);//подсчет КС посылки
	cmTrBuf0[3 + data_byte] = Low(CRC16);
	cmTrBuf0[4 + data_byte] = Hi(CRC16);
	return (5 + data_byte);
} //end Func01(void)




//реализация функции 0х02
//чтение значений из нескольких регистров флагов (Read Input Status)
char Func02(void)
{
	unsigned int CRC16;
	unsigned char i;

	//проверка корректного адреса в запросе и количество регистров на чтение
	char err = check_request(start_adress, quantity_registers);
	if (err != ERR_NONE)
		return ErrorMessage(err);

	//формируем ответ
	cmTrBuf0[1] = 0x02;//команда
	cmTrBuf0[2] = data_byte;//количество байт данных в ответе

	for (i = 0; i < quantity_registers; i++)
	{
		cmTrBuf0[3 + i / 8] |= ((bool)(NextBit(1x, i)) << (i % 8)); //данные
	}//end for()
	CRC16 = GetCRC16(cmTrBuf0, data_byte + 3);//подсчет КС посылки
	cmTrBuf0[3 + data_byte] = Low(CRC16);
	cmTrBuf0[4 + data_byte] = Hi(CRC16);
	return (5 + data_byte);
} //end Func02(void)




//реализация функции 0х03
//чтение значений из нескольких регистров хранения (Read Holding Registers)
char Func03(void)
{
	unsigned int CRC16;
	unsigned char i;

	//проверка корректного адреса в запросе и количество регистров на чтение
	char err = check_request(start_adress, quantity_registers);
	if (err != ERR_NONE)
		return ErrorMessage(err); //Адрес данных, указанный в запросе, недоступен

	  //формируем ответ
	cmTrBuf0[1] = 0x03;//команда
	cmTrBuf0[2] = data_2byte;//количество байт данных в ответе
	for (i = 0; i < quantity_registers; i++)
	{
		cmTrBuf0[3 + 2 * i] = Hi(RegNum4x[start_adress + i]);//данные старший байт
		cmTrBuf0[4 + 2 * i] = Low(RegNum4x[start_adress + i]);//данные младший байт
	} //end for()
	CRC16 = GetCRC16(cmTrBuf0, data_2byte + 3);//подсчет КС посылки
	cmTrBuf0[3 + data_2byte] = Low(CRC16);
	cmTrBuf0[4 + data_2byte] = Hi(CRC16);
	return (5 + data_2byte);
} //end Func03(void)




//реализация функции 0х04
//чтение значения аналоговых входов (Read Input Registers)
char Func04(void)
{
	unsigned int CRC16;
	unsigned char i;

	//проверка корректного адреса в запросе и количество регистров на чтение
	char err = check_request(start_adress, quantity_registers);
	if (err != ERR_NONE)
		return ErrorMessage(err); //Адрес данных, указанный в запросе, недоступен

	  //формируем ответ
	cmTrBuf0[1] = 0x04;//команда
	cmTrBuf0[2] = data_2byte;//количество байт данных в ответе
	for (i = 0; i < quantity_registers; i++)
	{
		cmTrBuf0[3 + 2 * i] = Hi(RegNum3x[start_adress + i]);//данные старший байт
		cmTrBuf0[4 + 2 * i] = Low(RegNum3x[start_adress + i]);//данные младший байт
	} //end for()
	CRC16 = GetCRC16(cmTrBuf0, data_2byte + 3);//подсчет КС посылки
	cmTrBuf0[3 + data_2byte] = Low(CRC16);
	cmTrBuf0[4 + data_2byte] = Hi(CRC16);
	return (5 + data_2byte);
} //end Func04(void)




//реализация функции 0х05
//запись значения одного флага (Force Single Coil)
char Func05(void)
{
	//проверка корректного адреса в запросе
	//проверка корректного адреса в запросе и количество регистров на чтение
	char err = check_request(start_adress, 1);
	if (err != ERR_NONE)
		return ErrorMessage(err); //Адрес данных, указанный в запросе, недоступен

	  //Значение FF 00 hex устанавливает флаг(1).
	  //Значение 00 00 hex сбрасывает флаг (0). 
	  //Другие значения не меняют флаг
	  //формируем ответ, возвращая полученное сообщение
	if ((value == 0xFF00) || (value == 0x0000))
	{
		if (value)
		{
			RegNum0x[I_byte] |= (1 << I_bit);
		}
		else
		{
			RegNum0x[I_byte] &= ~(1 << I_bit);
		}
		cmTrBuf0[1] = cmRcBuf0[1];
		cmTrBuf0[2] = cmRcBuf0[2];
		cmTrBuf0[3] = cmRcBuf0[3];
		cmTrBuf0[4] = cmRcBuf0[4];
		cmTrBuf0[5] = cmRcBuf0[5];
		cmTrBuf0[6] = cmRcBuf0[6];
		cmTrBuf0[7] = cmRcBuf0[7];
		return 8;
	} //end if()
	return ErrorMessage(0x03); //Значение, содержащееся в поле данных запроса, является недопустимой величиной
} //end Func05(void)




//реализация функции 0х06
//запись значения в один регистр хранения (Preset Single Register)
char Func06(void)
{
	//проверка корректного адреса в запросе и количество регистров на чтение
	char err = check_request(start_adress, 1);
	if (err != ERR_NONE)
		return ErrorMessage(err); //Адрес данных, указанный в запросе, недоступен

	  //формируем ответ, возвращая полученное сообщение
	if (1)//при необходимости поставить контроль значений
	{
		RegNum4x[reg_adress] = value;
		cmTrBuf0[1] = cmRcBuf0[1];
		cmTrBuf0[2] = cmRcBuf0[2];
		cmTrBuf0[3] = cmRcBuf0[3];
		cmTrBuf0[4] = cmRcBuf0[4];
		cmTrBuf0[5] = cmRcBuf0[5];
		cmTrBuf0[6] = cmRcBuf0[6];
		cmTrBuf0[7] = cmRcBuf0[7];
		return 8;
	} //end if()
	else
	{
		return ErrorMessage(0x03);//Значение, содержащееся в поле данных запроса, является недопустимой величиной
	}
	return 0;
} //end Func06(void)


//реализация функции 0х10
//Запись значений в несколько регистров хранения (Write Holding Registers)
char Func16(void)
{
	//проверка корректного адреса в запросе и количество регистров на чтение
	char err = check_request(start_adress, quantity_registers);
	if (err != ERR_NONE)
		return ErrorMessage(err); //Адрес данных, указанный в запросе, недоступен

	  //при необходимости поставить контроль значений
	if (!true)
	{//Одно из значений, содержащееся в поле данных запроса, является недопустимой величиной
		return ErrorMessage(ERR_ILLEGAL_DATA_VALUE);
	}

	for (int i = 0; i < num_to_write / 2; i++)
	{
		unsigned char* p = data_pointer + i * 2;
		RegNum4x[start_adress + i] = ((*p) << 8) + *(p + 1);
	}

	unsigned int CRC16;
	//формируем ответ
	cmTrBuf0[1] = cmRcBuf0[1];
	cmTrBuf0[2] = cmRcBuf0[2];
	cmTrBuf0[3] = cmRcBuf0[3];
	cmTrBuf0[4] = cmRcBuf0[4];
	cmTrBuf0[5] = cmRcBuf0[5];
	CRC16 = GetCRC16(cmTrBuf0, 6);//подсчет КС посылки
	cmTrBuf0[6] = Low(CRC16);
	cmTrBuf0[7] = Hi(CRC16);
	return 8;
} //end Func16(void)





//-----------------------------Прерывания-------------------------------

//прерывание по завершению получения данных 
//получение запроса от мастера
ISR(USART_RXC_vect)
{
	char cTempUART;

	cTempUART = UDR;

	if (UCSRA & (1 << FE)) //FE-ошибка кадра   ***может не работать на других МК****
	{
		UDR = 0xFE;
		return;
	}

	if (!StartRec)//если это первый байт, то начинаем прием
	{
		StartRec = true;
		RcCount = 0;
		cmRcBuf0[RcCount++] = cTempUART;
		StartTimer0();
	}
	else  // (StartRec==0) //продолжаем прием
	{
		if (RcCount < MAX_LENGHT_REC_BUF) //если еще не конец буфера
		{
			cmRcBuf0[RcCount++] = cTempUART;
		}
		else //буфер переполнен
		{
			cmRcBuf0[MAX_LENGHT_REC_BUF - 1] = cTempUART;
		}
		StartTimer0();//перезапуск таймера
	} //end else if (StartRec==0)
} //end ISR(USART_RXC_vect)


//прерывание по опустошению регистра данных (UDR)
//отправка ответа
ISR(USART_UDRE_vect)
{
	if (TrCount < cNumTrByte0)
	{
		UDR = cmTrBuf0[TrCount];
		TrCount++;
	} //end if
	else
	{
		StopTrans();
		TrCount = 0;
	} //end else
}//end ISR(USART_UDRE_vect)


//прерывание таймера/счетчика 0 по переполнению
//проверка окончания приема посылки, тишина >=3,5 символа
ISR(TIMER0_OVF_vect)
{
	if (StartRec)
	{
		StartRec = false; //посылка принята
		cNumRcByte0 = RcCount;  //кол-во принятых байт
		bModBus = true;//
		StopTimer0();//остановим таймер
	} //end if
}//end ISR(TIMER0_OVF_vect)
