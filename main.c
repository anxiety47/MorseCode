#include "LPC17xx.h"                    // Device header
#include "Board_LED.h"
#include "GPIO_LPC17xx.h"               // Keil::Device:GPIO
#include "PIN_LPC17xx.h"                // Keil::Device:PIN
#include "Open1768_LCD.h"
#include "LCD_ILI9325.h"
#include "asciiLib.h"
#include "string.h"
#include "math.h"

#define PI 3.1415926535

// deklaracje funkcji (def. ponizej maina
// kodowanie Morse'a i wysylanie/czytanie UART
void UART_init(void);
void sendCharToUART(char c);
void sendStringToUART(char* tab);
char readCharFromUART(void);
void fillMorseTab(void);
char* translate(char ch);
void readWordToTable(void);

//DAC
void TIMER1_IRQHandler(void);
void TIMER0_IRQHandler(void);

void TIM_init(void);
void initSinus(int amplitude);
void PinConfigure_forDAC(void);

//ustawienie pinu 2.0 jako output dla sygnalu prostokatnego Morse'a
void setOutputPin(void);
//Wyswietlacz
void screen_config(void); //konfiguracja pinow
void piszNaWyswietlaczu(char napis,int pozX, int pozY);
void napisNaWyswietlacz(char* word, int length,int startX, int startY);
void morseNaWyswietlacz(char* word,int length, int startX, int startY);
////////////////////////////////////////////////////////////////////////

//GLOBALNE ZMIENNE
char morseTab[37][6];
int howmanyletters = 50;
char textFromUART [50][6];
unsigned int value=0; 
int sinuses[100];

bool tiktak = true;

// do MR0 timer1
int sound_long = 25000000;
int sound_short = 12500000;

//nawigacja po tablicy textFromUART
int which_sign = 0; //ktory znak Morse'a w ramach jednej litery
int which_letter = 0; // ktora litera w slowie
/////////////////////////////////////////////////////////////////////////

// sprawdzanie ktorej biblioteki uzywamy do ekranu
void sprawdz(void)
{
	uint16_t osc = lcdReadReg(OSCIL_ON);
	if(osc == 0x8989)
	{
		while((LPC_UART0->LSR & (1 << 5)) == 0) continue;
				LPC_UART0->THR = 's';
	}
	else if(osc==0x9325 || osc==0x9328)
	{
		while((LPC_UART0->LSR & (1 << 5)) == 0) continue;
				LPC_UART0->THR = 'i';
	}
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                                                /* MAIN */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int main(void)
{
	screen_config();
	UART_init();

	//LCD z wykorzystaniem biblioteki (Open1768_LCD)
	//konfiguracja lcd
	lcdConfiguration();
	//sprawdz();
	init_ILI9325();
	//CALY EKRAN NA BIALO
	uint16_t bgcolor = LCDWhite;
	lcdSetCursor(0, 0);
	lcdWriteReg(DATA_RAM, 0);
	int i;
	for(i=0; i<240*320; i++)
		lcdWriteData(bgcolor);
	
	fillMorseTab(); 

	setOutputPin();
	readWordToTable();

	for (int i = 0; i < howmanyletters; i++)
	{
		 sendStringToUART(textFromUART[i]);
		 sendCharToUART(' ');

	}
	

	TIM_init();

	PinConfigure_forDAC();

	initSinus(2);

	while(1) 
	{

	}

}
///////////////////////////////////////////////////////


void sendCharToUART(char c)
{
    while((LPC_UART0->LSR & (1 << 5)) == 0) ;
		LPC_UART0->THR = c;
}

void sendStringToUART(char* tab)
{
    int length = strlen(tab);
    for (int i = 0; i < length; i++)
        sendCharToUART(tab[i]);
}

char readCharFromUART(void)
{
    char x;
    while((LPC_UART0->LSR & (1 << 0)) == 0){} ;
        x = LPC_UART0->RBR;
    return x;


}
/////////////////////////
void fillMorseTab(void)
{
	int counter = 0;
	for (int i = 97; i <  123 ; i++)
	{
		strcpy(morseTab[counter],translate((char)i));
		counter++;
	}
	for (int i = 48; i<58;i++)
	{
		strcpy(morseTab[counter],translate((char)i));
		counter++;
	}
	strcpy(morseTab[counter],translate(' '));


}
////////////////////////////
char* translate(char ch)
{

	switch( ch )
    {
    case 'a': case 'A':
        return ".-";				 //morseTab[0] = {'.'  ,  '-'  ,  '\0'};
    case 'b': case 'B':
        return "-...";
    case 'c': case 'C':
        return "-.-.";
    case 'd': case 'D':
        return "-..";
    case 'e': case 'E':
        return ".";
    case 'f': case 'F':
        return "..-.";
    case 'g': case 'G':
        return "--.";
    case 'h': case 'H':
        return "....";
    case 'i': case 'I':
        return "..";
    case 'j': case 'J':
        return ".---";
    case 'k': case 'K':
        return "-.-";
    case 'l': case 'L':
        return ".-..";
    case 'm': case 'M':
        return "--";
    case 'n': case 'N':
        return "-.";
    case 'o': case 'O':
        return "---\0";
    case 'p': case 'P':
        return ".--.";
    case 'q': case 'Q':
        return "--.-";
    case 'r': case 'R':
        return ".-.";
    case 's': case 'S':
        return "...\0";
    case 't': case 'T':
        return "-";
    case 'u': case 'U':
        return "..-";
    case 'v': case 'V':
        return "...-";
    case 'w': case 'W':
        return ".--";
    case 'x': case 'X':
        return "-..-";
    case 'y': case 'Y':
        return "-.--";
    case 'z': case 'Z':
        return "--..";
    case '1':
        return ".----";
    case '2':
        return "..---";
    case '3':
        return "...--";
    case '4':
        return "....-";
    case '5':
        return ".....";
    case '6':
        return "-....";
    case '7':
        return "--...";
    case '8':
        return "---..";
    case '9':
        return "----.";
    case '0':
        return "-----";
		case ' ':
				return "/";
		default:
				return "!";
    }

}

void readWordToTable(void)
{
	//textFromUART
	char x;
	char tableForWord[50] ;
	int counter = 0;
	char * tmp;
	while (1) 
	{
		x = readCharFromUART();
		
		
		if(x== 13)
			break;
		if(counter>=50) break;
		tableForWord[counter] = x;
		tmp = translate(x);
		strcpy(textFromUART[counter], tmp);
		counter++;

	}
	
	napisNaWyswietlacz(tableForWord, counter,10, 10);
}



/////////////////////////////////////////////////////////////////////
////////////////////    DAC /////////////////////////////////////////
/////////////////////////////////////////////////////////////////////


void TIMER0_IRQHandler(void)
{
 LPC_DAC->DACR = (sinuses[value]<<6);
		value++;
		if(value>=100) 
				value=0;
	LPC_TIM0->IR = 1;
}

void TIMER1_IRQHandler (void)
{
		if(tiktak == 0) 
			{
				if (textFromUART[which_letter][which_sign] == '.')
				{
					tiktak = 1;
					LPC_TIM0->TCR = 1;
					LPC_TIM1->MR0 = sound_short;
					LPC_GPIO2->FIOSET = 0xffffffff;
					which_sign++;

				}
				else if  (textFromUART[which_letter][which_sign] == '-')
				{
					tiktak = 1;
					LPC_TIM0->TCR = 1;
					LPC_TIM1->MR0 = sound_long;
					LPC_GPIO2->FIOSET = 0xffffffff;
					which_sign++;

				}
				
				else if  (textFromUART[which_letter][which_sign] == '/')
				{
					which_letter++;
					which_sign = 0;
					LPC_TIM0->TCR = 0;
					LPC_TIM1->MR0 = 1.5*sound_long;
					//LPC_GPIO2->FIOCLR = 0xffffffff;
					LPC_GPIO2->FIOCLR |= (1<<0);
				}
				else 
				{	
					which_letter++;
					which_sign = 0;
				}
				
			}
		else 
		{
			tiktak = 0;
      LPC_TIM0->TCR = 0;
			LPC_TIM1->MR0 = sound_short;
			//LPC_GPIO2->FIOCLR = 0xffffffff;
			LPC_GPIO2->FIOCLR |= (1<<0);

		}

		sendStringToUART("");
	
		LPC_TIM1->IR = (1<<0);
}


void TIM_init(void)
{
	LPC_TIM0->PR = 0;
	//ustawianie bitow
	LPC_TIM0->MCR = (1<<0) | (1<<1);
	LPC_TIM1->MCR = 3;
	
	LPC_TIM0->MR0 = 250;
	LPC_TIM1->MR0 = 25000000;
	// dzwiek 2 razy krotszy:
	//LPC_TIM1->MR0 = 25000000/2;

	
	NVIC_EnableIRQ(TIMER0_IRQn);
	NVIC_EnableIRQ(TIMER1_IRQn);

	LPC_TIM0->TCR = 0;
	LPC_TIM1->TCR = 1;
	LPC_GPIO2->FIOCLR |= (1<<0);
	
}

void initSinus(int amplitude) 
{
	for(int i=0; i<100; i++)
		sinuses[i] = amplitude * sin(i* 2*PI / 100.0) + 512;
}

void PinConfigure_forDAC(void)
{
    PIN_Configure(0, 26, 2 , 2, 0);
}
//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////

// konfigurowanie UARTu
void UART_init(void)
{
	LPC_UART0->LCR = (1<<7) | (3<<0);
	LPC_UART0->DLL = 13;
	LPC_UART0->DLM = 0;
	LPC_UART0->LCR = 3;

	PIN_Configure (0, 3, 1, PIN_PINMODE_TRISTATE, 0);
	PIN_Configure (0, 2, 1, PIN_PINMODE_TRISTATE, 0);

}

//wlaczenie pinu 2.0 jako output, ustawienie poczatkowo stanu niskiego
void setOutputPin(void)
{
	  LPC_PINCON->PINSEL4 = 0x000000;  //gpio
    LPC_GPIO2->FIODIR = 0xffffffff;  //output
	  //LPC_GPIO2->FIOCLR = 0xffffffff;
	  LPC_GPIO2->FIOCLR |= (1<<0); 
}

// funkcja wypisujaca litere na wyswietlacz

void piszNaWyswietlaczu(char napis,int pozX, int pozY) 
{
	unsigned char buffer[16];
	GetASCIICode(1, buffer, napis);
	for (int i = 0; i < 16; ++i) 
	{

		//zeby auto inkrementacja nei leciala w jednej linii
		lcdSetCursor(pozX, pozY+i);
		lcdWriteReg(DATA_RAM, pow(2,16)-1);
		for (int j = 7; j >= 0; j--) 
		{

			//bez rzucenia koloru na writereg auto inkrementacja nie zadziala
				if (buffer[i] & (1 << j)) 
					 lcdWriteData(0); //czarny kolor
				else
					lcdWriteData(pow(2,16)-1);
		}
	}
}

void napisNaWyswietlacz(char* word,int length, int startX, int startY)
{
	int offsetX = 0;
	int offsetY = 0;
	for (int i = 0; i < length ;i++)
	{
		if (startY + offsetY > 310)
			break;
		
		piszNaWyswietlaczu(word[i], startX + offsetX, startY + offsetY);
		offsetX +=10;
		if (startX + offsetX > 220)
		{
			offsetX = 0;
			offsetY += 20;
		}
	}
	
	offsetX = 0;
	for (int i = 0; i < length; i++)
	{
		int morseLength = strlen(textFromUART[i]);
		if (startX + offsetX > 180)
		{
			offsetX = 0;
			offsetY += 20;
		}
		morseNaWyswietlacz(textFromUART[i], morseLength,startX + offsetX, startY + offsetY + 20);

		offsetX += morseLength * 10 + 10;
		
	}
}

void morseNaWyswietlacz(char* word,int length, int startX, int startY)
{
	int offsetX = 0;
	int offsetY = 0;
	for (int i = 0; i < length ;i++)
	{
		if (startY + offsetY > 310)
			break;

		piszNaWyswietlaczu(word[i], startX + offsetX, startY + offsetY);
		offsetX +=10;
		if (startX + offsetX > 220)
		{
			offsetX = 0;
			offsetY += 20;
		}
	}
	
}

// konfiguracja pinow (ekran)
void screen_config(void)
{
	PIN_Configure (0, 19, 0, PIN_PINMODE_PULLUP, 0);

	LPC_GPIO0->FIODIR &= (0 << 19); //1
	LPC_GPIOINT->IO0IntEnR |= (1<<19); //1
	NVIC_EnableIRQ(EINT3_IRQn);

}
