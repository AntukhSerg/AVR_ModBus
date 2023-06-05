//ATmega8
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdbool.h>
#include <util/crc16.h>
#include <stdint.h>
#include "AVR_ModBus_GT.h"

#define Hi(Int) (char) (Int>>8)
#define Low(Int) (char) (Int)

uint8_t RcCount, TrCount;  //������� ��������/���������� ������
bool StartRec = false;// false/true ������/����� �������
bool bModBus = false;  //���� ��������� �������
uint8_t cNumRcByte0; //���-�� �������� ����
uint8_t cNumTrByte0;  //���-�� ������������ ����
uint8_t cmRcBuf0[MAX_LENGHT_REC_BUF] ; //����� ����������� ������
uint8_t cmTrBuf0[MAX_LENGHT_TR_BUF] ; //����� ������������ ������


uint8_t ModBus(unsigned char NumByte);
char Func01(void);
char Func02(void);
char Func03(void);
char Func04(void);
char Func05(void);
char Func06(void);
char ErrorMessage(char Error);
uint16_t GetCRC16(uint8_t *buf, uint8_t bufsize);

#define period_us 100 //���������� ����������� �� 1 ������ ����������� �������
#define count_us (256-period_us) 

//�������� ����� 3,5������� ��� UART
//t3_5= 3.5*(1+8+1+1)/baud
// ���� baud >= 19200, �� t3_5 = 1.75ms
//��� baud = 9600   t3_5 = 4.0ms
uint8_t delay3_5 = 40; // ticks GT, 4ms/0.1ms
uint32_t Next_Time;



//������������� ������� � UART
void InitModBus(void)
{
//��������� UART
UBRRHi = Hi(BAUD_DIVIDER);
UBRRLow = Low(BAUD_DIVIDER);
UCSRA_CLR();
UCSRB_SET();
UCSRC_SET();

//������ ������� ����������� �������
GLOBAL_TIME_START();
G_TIME = 0;

}//end InitModBus()


void CheckModBus(void)
{
if (bModBus)
	{
	cNumTrByte0=ModBus(cNumRcByte0); //��������� ��������� ���������� ModBus
	if (cNumTrByte0!=0)
		{
		TrCount=0;
		StartTrans();
		} //end if (cNumTrByte0!=0)
	bModBus=false;
	} //end if (bModBus)
} //end CheckModBus(void)




uint8_t ModBus(uint8_t NumByte)
{
int CRC16;
uint8_t i;

if (cmRcBuf0[0]!=SLAVE_ID) return 0x00; //broadcast ������, ����� �� �����

CRC16 = GetCRC16(cmRcBuf0, NumByte);
if (CRC16) return 0;  //����������� ����� �� ���������, ����� �� �����

//�������� ������ ������� �������� ����������� ����� �� ������
//������ ��� ������ ��������� � ������ cmTrBuf0[]
//!!!!! ������ � ������ ���������� ��������� ������� ������ ������ (Hi, Low)
//!!!!! CRC ���������� ������� ������ ������ (Low, Hi) 

cmTrBuf0[0]=SLAVE_ID;//����� ����������

for(i=1; i<(MAX_LENGHT_TR_BUF); i++) //������� ����� ��� ������
	{
	cmTrBuf0[i]=0;
	}//end for()

//��� �������
switch(cmRcBuf0[1]) 
	{
    case 0x01: 
		{
		if(!QUANTITY_REG_0X) return ErrorMessage(0x01);
		return Func01();
		}
	
	case 0x02: 
		{
		if(!QUANTITY_REG_1X) return ErrorMessage(0x01);
		return Func02();
		}
		
	case 0x03: 
		{
		if(!QUANTITY_REG_4X) return ErrorMessage(0x01);
		return Func03();
		}
		
	case 0x04: 
		{
		if(!QUANTITY_REG_3X) return ErrorMessage(0x01);
		return Func04();
		}
		
    case 0x05: 
		{
		if(!QUANTITY_REG_0X) return ErrorMessage(0x01);
		return Func05();
		}
		
	case 0x06: 
		{
		if(!QUANTITY_REG_4X) return ErrorMessage(0x01);
		return Func06();
		}
		
    default:   return ErrorMessage(0x01); //�������� ��� ������� �� ����� ���� ���������
	} //end switch
} //end ModBus()



//������� ��������� ��� CRC-16
//�� ����� ��������� �� ������ ������
//� ���������� ���� ��������� (��� ��������� ���� CRC-16)
uint16_t GetCRC16(uint8_t *buf, uint8_t bufsize)
{
uint16_t crc = 0xFFFF;
uint8_t i;

for (i = 0; i < bufsize; i++) 
crc = _crc16_update(crc, buf[i]);//������� CRC � �������� �������
return crc;
} //end GetCRC16()



//������������ ������ �� ������
char ErrorMessage(char Error)
{
int CRC16;

cmTrBuf0[1]=cmRcBuf0[1]+0x80;//������� � �������
cmTrBuf0[2]=Error;
CRC16=GetCRC16(cmTrBuf0,3);//������� �� �������
cmTrBuf0[3]=Low(CRC16);
cmTrBuf0[4]=Hi(CRC16);

return 5;
} //end ErrorMessage()



//---------------------------������� ModBus-------------------------------

#define start_adress ((cmRcBuf0[2]<<8) + cmRcBuf0[3])
#define quantity_registers ((cmRcBuf0[4]<<8) + cmRcBuf0[5])
#define data_byte ((quantity_registers+7)/8)
#define data_2byte (2*quantity_registers) //������ �����������
#define max_adress (start_adress + quantity_registers)
#define reg_adress (start_adress)
#define value (quantity_registers)
//��������� ������
#define I_byte (reg_adress/8)
#define I_bit (reg_adress%8)
//��������� ������ � ������ �������� ������
#define I_byte_ofset(x) ((start_adress+x)/8)
#define I_bit_ofset(x) ((start_adress+x)%8)
#define NextBit(y,x) RegNum##y[I_byte_ofset(x)] & (1<<I_bit_ofset(x))	
#define ClrBitOfByte(ByteA, x) (ByteA &= ~(1<<(x)))



//���������� ������� 0�01
//������ �������� �� ���������� ��������� ������ (Read Coil Status)
char Func01(void)
{
uint16_t CRC16;
uint8_t i;

//�������� ����������� ������ � �������
if(!(max_adress<=QUANTITY_REG_0X)) return ErrorMessage(0x02); //����� ������, ��������� � �������, ����������
if(!quantity_registers) return 0;//��������� ��� ���������� ��������� �� ������ > 0

//��������� �����
cmTrBuf0[1] = 0x01;//�������
cmTrBuf0[2] = data_byte;//���������� ���� ������ � ������

for(i=0; i<quantity_registers; i++) 
	{
	cmTrBuf0[3+i/8] |= ((bool)(NextBit(0x,i))<<(i%8)); //������
	}//end for()
CRC16=GetCRC16(cmTrBuf0,data_byte+3);//������� �� �������
cmTrBuf0[3+data_byte]=Low(CRC16);
cmTrBuf0[4+data_byte]=Hi(CRC16);
return (5+data_byte);
} //end Func01(void)




//���������� ������� 0�02
//������ �������� �� ���������� ��������� ������ (Read Input Status)
char Func02(void)
{
uint16_t CRC16;
uint8_t i;

//�������� ����������� ������ � �������
if(!(max_adress<=QUANTITY_REG_1X)) return ErrorMessage(0x02); //����� ������, ��������� � �������, ����������
if(!quantity_registers) return 0;//��������� ��� ���������� ��������� �� ������ > 0

//��������� �����
cmTrBuf0[1] = 0x02;//�������
cmTrBuf0[2] = data_byte;//���������� ���� ������ � ������

for(i=0; i<quantity_registers; i++) 
	{
	cmTrBuf0[3+i/8] |= ((bool)(NextBit(1x,i))<<(i%8)); //������
	}//end for()
CRC16=GetCRC16(cmTrBuf0,data_byte+3);//������� �� �������
cmTrBuf0[3+data_byte]=Low(CRC16);
cmTrBuf0[4+data_byte]=Hi(CRC16);
return (5+data_byte);
} //end Func02(void)




//���������� ������� 0�03
//������ �������� �� ���������� ��������� �������� (Read Holding Registers)
char Func03(void)
{
uint16_t CRC16;
uint8_t i;

//�������� ����������� ������ � �������
if(!(max_adress<=QUANTITY_REG_4X)) return ErrorMessage(0x02); //����� ������, ��������� � �������, ����������
if(!quantity_registers) return 0;//��������� ��� ���������� ��������� �� ������ > 0 

//��������� �����
cmTrBuf0[1] = 0x03;//�������
cmTrBuf0[2] = data_2byte;//���������� ���� ������ � ������
for(i=0; i<quantity_registers; i++)
	{
	cmTrBuf0[3+2*i] = Hi(RegNum4x[start_adress+i]);//������ ������� ����
	cmTrBuf0[4+2*i] = Low(RegNum4x[start_adress+i]);//������ ������� ����
	} //end for()
CRC16=GetCRC16(cmTrBuf0,data_2byte+3);//������� �� �������
cmTrBuf0[3+data_2byte]=Low(CRC16);
cmTrBuf0[4+data_2byte]=Hi(CRC16);
return (5+data_2byte);
} //end Func03(void)




//���������� ������� 0�04
//������ �������� ���������� ������ (Read Input Registers)
char Func04(void)
{
uint16_t CRC16;
uint8_t i;

//�������� ����������� ������ � �������
if(!(max_adress<=QUANTITY_REG_3X)) return ErrorMessage(0x02); //����� ������, ��������� � �������, ����������
if(!quantity_registers) return 0;//��������� ��� ���������� ��������� �� ������ > 0 

//��������� �����
cmTrBuf0[1] = 0x04;//�������
cmTrBuf0[2] = data_2byte;//���������� ���� ������ � ������
for(i=0; i<quantity_registers; i++)
	{
	cmTrBuf0[3+2*i] = Hi(RegNum3x[start_adress+i]);//������ ������� ����
	cmTrBuf0[4+2*i] = Low(RegNum3x[start_adress+i]);//������ ������� ����
	} //end for()
CRC16=GetCRC16(cmTrBuf0,data_2byte+3);//������� �� �������
cmTrBuf0[3+data_2byte]=Low(CRC16);
cmTrBuf0[4+data_2byte]=Hi(CRC16);
return (5+data_2byte);
} //end Func04(void)




//���������� ������� 0�05
//������ �������� ������ ����� (Force Single Coil)
char Func05(void)
{
//�������� ����������� ������ � �������
if(!(reg_adress<=QUANTITY_REG_0X)) return ErrorMessage(0x02); //����� ������, ��������� � �������, ����������

//�������� FF 00 hex ������������� ����(1).
//�������� 00 00 hex ���������� ���� (0). 
//������ �������� �� ������ ����
//��������� �����, ��������� ���������� ���������
if((value==0xFF00)||(value==0x0000))
	{
	if(value)
		{
		RegNum0x[I_byte] |= (1<<I_bit);
		}
	else 
		{
		RegNum0x[I_byte] &= ~(1<<I_bit);
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
return ErrorMessage(0x03); //��������, ������������ � ���� ������ �������, �������� ������������ ���������
} //end Func05(void)




//���������� ������� 0�06
//������ �������� � ���� ������� �������� (Preset Single Register)
char Func06(void)
{
//�������� ����������� ������ � �������
if(!(reg_adress<=QUANTITY_REG_4X)) return ErrorMessage(0x02); //����� ������, ��������� � �������, ����������
//�������� ���������� ������ � �������

//��������� �����, ��������� ���������� ���������
if(1)//��� ������������� ��������� �������� ��������
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
	return ErrorMessage(0x03);//��������, ������������ � ���� ������ �������, �������� ������������ ���������
	}
return 0;
} //end Func06(void)




//-----------------------------����������-------------------------------

//���������� �� ���������� ��������� ������ 
//��������� ������� �� �������
ISR(USART_RXC_vect) 
{
char cTempUART;

cTempUART=UDR;

if  (UCSRA&(1<<FE)) //FE-������ �����   ***����� �� �������� �� ������ ��****
	{
	UDR = 0xFE; 
	return;
	}   

if (!StartRec)//���� ��� ������ ����, �� �������� �����
	{
	StartRec=true;
	RcCount=0;
	cmRcBuf0[RcCount++]=cTempUART;
	Next_Time = G_TIME + delay3_5;
	}
else  // (StartRec==0) //���������� �����
	{
    if (RcCount<MAX_LENGHT_REC_BUF) //���� ��� �� ����� ������
		{
		cmRcBuf0[RcCount++]=cTempUART;
		}
	else //����� ����������
		{
		cmRcBuf0[MAX_LENGHT_REC_BUF-1]=cTempUART;
		}
    Next_Time = G_TIME + delay3_5;
	} //end else if (StartRec==0)
} //end ISR(USART_RXC_vect)


//���������� �� ����������� �������� ������ (UDR)
//�������� ������
ISR(USART_UDRE_vect)
{
if (TrCount<cNumTrByte0)
	{
    UDR=cmTrBuf0[TrCount];
    TrCount++;
	} //end if
else
	{
    StopTrans();
    TrCount=0;
	} //end else
}//end ISR(USART_UDRE_vect)


//���������� �������/�������� 0 �� ������������
//�������� ��������� ������ �������, ������ >=3,5 �������
ISR(TIMER0_OVF_vect)
{
G_TIME++;
if ((G_TIME >= Next_Time)&(StartRec))
	{
    StartRec=false; //������� �������
    cNumRcByte0=RcCount;  //���-�� �������� ����
	bModBus=true;//
    } //end if
TCNT0 = count_us+TCNT0;
}//end ISR(TIMER0_OVF_vect)

