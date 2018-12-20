#define _XTAL_FREQ 10000000

#include <xc.h>
#include<pic18f4550.h>
#include <stdio.h>
#include <stdlib.h>

#pragma config FCMEN=ON
#pragma config WDT=OFF
#pragma config IESO=ON
#pragma config XINST=OFF
#pragma config BOR = ON         // Brown-out Reset Enable bits (Brown-out Reset enabled

// BEGIN CONFIG
#pragma config FOSC = HS // Oscillator Selection bits (HS oscillator)
#pragma config PWRT = OFF        // Power-up Timer Enable bit (PWRT enabled)
#pragma config LVP = OFF // Low-Voltage (Single-Supply) In-Circuit Serial Programming Enable bit (RB3 is digital I/O, HV on MCLR must be used for programming)
#pragma config CPD = OFF // Data EEPROM Memory Code Protection bit (Data EEPROM code protection off)
//END CONFIG

#define uchar unsigned char
#define uint unsigned int

#define LCD_RS LATDbits.LATD2
#define LCD_EN LATDbits.LATD3
#define PORT_D LATB

#define SWPORTdir TRISD
#define SWPORT PORTD
#define enrol RD4
#define match RD5
#define delet RD7

#define ok RD6
#define up RD5
#define down RD4


#define HIGH 1
#define LOW 0

#define PASS 0
#define ERROR 1

#define checkKey(id) id=up<down?++id:down<up?--id:id;

void lcdinst();
void delay_1(int ms);
void clk();
void LCD_init();

void LCD_char(unsigned char c);
void LCD_command(unsigned char c);
void delay_2(unsigned int val);
void LCD_String(const char *msg);
void RC_ISR(void);

uchar buf[20];
uchar buf1[20];
volatile uint index=0;
volatile int flag=0;
uint msCount=0;
uint g_timerflag=1;
volatile uint count=0;
uchar data[10];
uint id=1;

enum
{
 CMD,
 DATA, 
 SBIT_CREN=4,
 SBIT_TXEN,
 SBIT_SPEN,
};

 

const char passPack[]={0xEF, 0x1, 0xFF, 0xFF, 0xFF, 0xFF, 0x1, 0x0, 0x7, 0x13, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1B};
const char f_detect[]={0xEF, 0x1, 0xFF, 0xFF, 0xFF, 0xFF, 0x1, 0x0, 0x3, 0x1, 0x0, 0x5};
const char f_imz2ch1[]={0xEF, 0x1, 0xFF, 0xFF, 0xFF, 0xFF, 0x1, 0x0, 0x4, 0x2, 0x1, 0x0, 0x8};
const char f_imz2ch2[]={0xEF, 0x1, 0xFF, 0xFF, 0xFF, 0xFF, 0x1, 0x0, 0x4, 0x2, 0x2, 0x0, 0x9};
const char f_createModel[]={0xEF,0x1,0xFF,0xFF,0xFF,0xFF,0x1,0x0,0x3,0x5,0x0,0x9};
char f_storeModel[]={0xEF,0x1,0xFF,0xFF,0xFF,0xFF,0x1,0x0,0x6,0x6,0x1,0x0,0x1,0x0,0xE};
const char f_search[]={0xEF, 0x1, 0xFF, 0xFF, 0xFF, 0xFF, 0x1, 0x0, 0x8, 0x1B, 0x1, 0x0, 0x0, 0x0, 0xA3, 0x0, 0xC8};
char f_delete[]={0xEF,0x1,0xFF,0xFF,0xFF,0xFF,0x1,0x0,0x7,0xC,0x0,0x0,0x0,0x1,0x0,0x15};



void serialbegin(uint baudrate)
{
  //SPBRG = (18432000UL/(long)(64UL*baudrate))-1;      // baud rate @18.432000Mhz Clock
     TRISCbits.TRISC6 = 0; //TX pin = output
    TRISCbits.TRISC7 = 1; //RC pin = input

//    TXSTA = 0x20; //low baud rate, 8-bit
    SPBRG = 10; //9600 baud rate
    //SPBRG = (10000000UL/(long)(64UL*19200))-1;
    //SPBRG = 3; //9600 baud rate
    RCSTAbits.CREN = 1;
    RCSTAbits.SPEN = 1;
    TXSTAbits.TXEN = 1;
    TXSTAbits.BRGH = 1;

    PIE1bits.RCIE = 1; //enable rcv interrupt
    PIE1bits.TXIE = 0; //enable TX interrupt
    
    INTCONbits.PEIE=1;	//enable peripherals interrupt
	INTCONbits.GIE=1;	//enable all interrupts
  
}

void serialwrite(char ch)
{
    while(TXIF==0);  // Wait till the transmitter register becomes empty
    //TXIF=0;          // Clear transmitter flag
    TXREG=ch;        // load the char to be transmitted into transmit reg
}

serialprint(char *str)
{
    while(*str)
    {
        serialwrite(*str++);
    }
}

void interrupt SerialRxPinInterrupt(void)
{
    if((PIR1bits.RCIF == 1))
    {
        
        uchar ch=RCREG; 
        buf[index++]=ch;
        if(index>0)
            flag=1;
        RCIF = 0; // clear rx flag
    }  
}

void serialFlush()
{
    for(int i=0;i<sizeof(buf);i++)
    {
        buf[i]=0;
    }
}

int sendcmd2fp(char *pack, int len)
{
  uint res=ERROR;
  serialFlush();
  index=0;
  __delay_ms(100);
  for(int i=0;i<len;i++)
  {
    serialwrite(*(pack+i));
  }
  __delay_ms(1000);
  if(flag == 1)
  {
    if(buf[0] == 0xEF && buf[1] == 0x01)
    {
        if(buf[6] == 0x07)   // ack
        {
        if(buf[9] == 0)
        {
            uint data_len= buf[7];
            data_len<<=8;
            data_len|=buf[8];
            for(int i=0;i<data_len;i++)
                data[i]=0;
            for(int i=0;i<data_len-2;i++)
            {
                data[i]=buf[10+i];
            }
            res=PASS;
        }

        else
        {
         res=ERROR;
        }
        }
    }
    index=0;
    flag=0;
    return res;
}
}

uint getId()
{
    uint id=0;
     LCD_command(0x01);
     
    
    while(1)
    {
        //lcdwrite(0x80, CMD);
        
        checkKey(id);
        LCD_command(0x01);
        sprintf(buf1,"Enter Id:%d  ",id);
        LCD_String(buf1);
        __delay_ms(500);
        if(ok == LOW)
            return id;
    }
}

void matchFinger()
{
      LCD_command(0x01);
      LCD_String("Place Finger");
      //lcdwrite(192,CMD);
      __delay_ms(2000);
     if(!sendcmd2fp(&f_detect[0],sizeof(f_detect)))
     {
         if(!sendcmd2fp(&f_imz2ch1[0],sizeof(f_imz2ch1)))
         {
            if(!sendcmd2fp(&f_search[0],sizeof(f_search)))
            {
                LCD_command(0x01);
                 LCD_String("Finger Found");
                 __delay_ms(1000);
                 
                uint id= data[0];
                     id<<=8;
                     id+=data[1];
                uint score=data[2];
                        score<<=8;
                        score+=data[3];
                sprintf(buf1,"Id:%d  Score:%d",id,score);
                LCD_command(0x01);  
               LCD_String(buf1);
               PORTAbits.RA0=1;
                __delay_ms(1000);
                PORTAbits.RA0=0;
            }
            
            else
            {
               LCD_command(0x01);
                 LCD_String("Unvalid");
            }
         }
     }
      
     else
     {
        LCD_command(0x01);
        LCD_String("No finger");
     }
      __delay_ms(2000);
}

void enrolFinger()
{
     LCD_command(0x01);
     LCD_String("Enroll");
       __delay_ms(1000);
     LCD_command(0x01);
     LCD_String("Place Finger");
     __delay_ms(2000);
     if(!sendcmd2fp(&f_detect[0],sizeof(f_detect)))
     {
        if(!sendcmd2fp(&f_imz2ch1[0],sizeof(f_imz2ch1)))
        {
//            lcdprint("Finger Detected");
//            __delay_ms(1000);
//            lcdwrite(1,CMD);
//            lcdprint("Place Finger");
//            lcdwrite(192,CMD);
//            lcdprint("    Again   "); 
//            __delay_ms(2000);
            
                LCD_command(0x01);
                LCD_String("FP detected");
                __delay_ms(2000);
                 
                 LCD_command(0x01);
                 LCD_String("Again");
                 __delay_ms(2000);
            if(!sendcmd2fp(&f_detect[0],sizeof(f_detect)))
            {
                if(!sendcmd2fp(&f_imz2ch2[0],sizeof(f_imz2ch2)))
                {
                    LCD_command(0x01);
                    LCD_String("FP detected");
                    __delay_ms(1000);
                    if(!sendcmd2fp(&f_createModel[0],sizeof(f_createModel)))
                    {
                        id=getId();
                        f_storeModel[11]= (id>>8) & 0xff;
                        f_storeModel[12]= id & 0xff;
                        f_storeModel[14]= 14+id; 
                       if(!sendcmd2fp(&f_storeModel[0],sizeof(f_storeModel)))
                       {
                            LCD_command(0x01);
                            LCD_String("Stored");
                            __delay_ms(1000);
                            sprintf(buf1,"Id:%d",id);
                            LCD_command(0x01);
                            LCD_String(buf1);
                            __delay_ms(1000);
                            return;
                       }
                       
                       else
                       {
                            LCD_command(0x01);
                            LCD_String("FP not stored");
                       }
                    }
                    else
                        LCD_command(0x01);
                        LCD_String("error");
                }
                else
                   LCD_command(0x01);
                 LCD_String("error");
            }
            else
                LCD_command(0x01);
                 LCD_String("no FP");
                 return;
        } 
     }
     else
     {
        LCD_command(0x01);
        LCD_String("no fp"); 
        return;         
     }
      __delay_ms(2000);
}

void deleteFinger()
{
    id=getId();
   f_delete[10]=id>>8 & 0xff;
   f_delete[11]=id & 0xff;
   f_delete[14]=(21+id)>>8 & 0xff;
   f_delete[15]=(21+id) & 0xff;
   if(!sendcmd2fp(&f_delete[0],sizeof(f_delete)))
  {
     LCD_command(0x01); 
     sprintf(buf1,"Finger ID %d ",id);
    
      LCD_String(buf1); 
       __delay_ms(2000);
     LCD_command(0x01);
     LCD_String("deleted"); 
     
  }
   else
   {
       LCD_command(0x01);
     LCD_String("error"); 
   }
   __delay_ms(2000);
}
      

int main()
{            
  void (*FP)();  
  ADCON1=0b00000110;
 
  SWPORTdir=0xF0;
  SWPORT=0x0F;
  serialbegin(57600);
  TRISDbits.RD2 = 0; //lcd rs
  TRISDbits.RD3 = 0; //lcd en
  
  TRISAbits.RA0 = 0; //led connected valid fingerprint
  TRISDbits.RD0 = 1; //video started 
  TRISDbits.RD1 = 1; //screenshot
  
  TRISB = 0x00;

  LCD_EN = 1;
  delay_1(1);

  LCD_init();
 LCD_command(0x01);
     LCD_String("fingerprint"); 
  
  LCD_command(192);
     LCD_String("Interfacing"); 
  delay_1(1000);
  
  index=0;    
  while(sendcmd2fp(&passPack[0],sizeof(passPack)))
  {
    LCD_command(0x01);
     LCD_String("FP not found"); 
     __delay_ms(2000);
     index=0;
  }
  LCD_command(0x01);
     LCD_String("FP found"); 
  __delay_ms(1000);
  lcdinst();
  while(1)
  { 
    FP=match<enrol?matchFinger:enrol<delet?enrolFinger:delet<enrol?deleteFinger:lcdinst;
    FP();
  } 
  return 0;
}

void lcdinst(){
    if(PORTDbits.RD0 == 1){
        if(PORTDbits.RD1 == 1){
            LCD_command(0x01);
             LCD_String("Screenshot");
             __delay_ms(2000);
             return;
        }else{
            LCD_command(0x01);
             LCD_String("Video Started");
             __delay_ms(500);
             return;
        }
    
    } else{
        LCD_command(0x01);
         LCD_String("1-Match 2-Enroll");
         __delay_ms(500);
          LCD_command(0x01);
         LCD_String("3-delete Finger");
         __delay_ms(500);
       // __delay_ms(10);
    }
}

void LCD_init() {
    delay_1(15);
   // TRISD = 0x00;    /*Set PORTB as output PORT for LCD data(D0-D7) pins*/
    //TRISB = 0x00;    /*Set PORTD as output PORT LCD Control(RS,EN) Pins*/
    TRISB = 0;  //set PORTB (LCD data pins) as output
    TRISD3 = 0;  //set PORTD (LCD EN pin) as output
    TRISD2 = 0;  //set PORTD (LCD RS pin) as output
    
    LCD_EN = 0;
    
	LCD_command(0x38);     /*uses 2 line and initialize 5*7 matrix of LCD*/
    
    LCD_command(0x01);     /*clear display screen*/
    LCD_command(0x0c);     /*display on cursor off*/
    LCD_command(0x06);     /*increment cursor (shift cursor to right)*/
    LCD_command(0x80);     /*increment cursor (shift cursor to right)*/
}

void LCD_char(unsigned char dat) {
    PORT_D = dat;
    LCD_RS = 1;
    clk();
    delay_1(1);
}

void LCD_String(const char *msg)
{
	while((*msg)!=0)
	{		
	  LCD_char(*msg);
	  msg++;	
    }		
}

void LCD_command(unsigned char cmd) {
    PORT_D = cmd;
    LCD_RS = 0;
    clk();
    delay_1(1);
}

void delay_1(int ms) {

    int counter = 1;
    for (counter = 1; counter <= ms; ++counter) {
        delay_2(25);
    }

}

void delay_2(unsigned int val)
{
     unsigned int i,j;
        for(i=0;i<=val;i++)
            for(j=0;j<81;j++);      /*This count Provide delay of 1 ms for 8MHz Frequency */
 }

void clk() {
    LCD_EN = 0;
    delay_1(1);
    LCD_EN = 1;
}