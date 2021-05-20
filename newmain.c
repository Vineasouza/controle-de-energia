/*
 * File:   newmain.c
 * Author: lucas
 *
 * Created on 20 de Abril de 2021, 14:47
 */



// PIC18F4550 Configuration Bit Settings

// 'C' source line config statements

// CONFIG1L
#pragma config PLLDIV = 1       // PLL Prescaler Selection bits (No prescale (4 MHz oscillator input drives PLL directly))
#pragma config CPUDIV = OSC1_PLL2// System Clock Postscaler Selection bits ([Primary Oscillator Src: /1][96 MHz PLL Src: /2])
#pragma config USBDIV = 1       // USB Clock Selection bit (used in Full-Speed USB mode only; UCFG:FSEN = 1) (USB clock source comes directly from the primary oscillator block with no postscale)

// CONFIG1H
#pragma config FOSC = HS        // Oscillator Selection bits (HS oscillator (HS))
#pragma config FCMEN = OFF      // Fail-Safe Clock Monitor Enable bit (Fail-Safe Clock Monitor disabled)
#pragma config IESO = OFF       // Internal/External Oscillator Switchover bit (Oscillator Switchover mode disabled)

// CONFIG2L
#pragma config PWRT = ON       // Power-up Timer Enable bit (PWRT disabled)
#pragma config BOR = ON        // Brown-out Reset Enable bits (Brown-out Reset disabled in hardware and software)
#pragma config BORV = 3         // Brown-out Reset Voltage bits (Minimum setting 2.05V)
#pragma config VREGEN = OFF     // USB Voltage Regulator Enable bit (USB voltage regulator disabled)

// CONFIG2H
#pragma config WDT = OFF        // Watchdog Timer Enable bit (WDT disabled (control is placed on the SWDTEN bit))
#pragma config WDTPS = 32768    // Watchdog Timer Postscale Select bits (1:32768)

// CONFIG3H
#pragma config CCP2MX = OFF     // CCP2 MUX bit (CCP2 input/output is multiplexed with RB3)
#pragma config PBADEN = OFF     // PORTB A/D Enable bit (PORTB<4:0> pins are configured as digital I/O on Reset)
#pragma config LPT1OSC = OFF    // Low-Power Timer 1 Oscillator Enable bit (Timer1 configured for higher power operation)
#pragma config MCLRE = ON       // MCLR Pin Enable bit (MCLR pin enabled; RE3 input pin disabled)

// CONFIG4L
#pragma config STVREN = OFF     // Stack Full/Underflow Reset Enable bit (Stack full/underflow will not cause Reset)
#pragma config LVP = OFF        // Single-Supply ICSP Enable bit (Single-Supply ICSP disabled)
#pragma config ICPRT = OFF      // Dedicated In-Circuit Debug/Programming Port (ICPORT) Enable bit (ICPORT disabled)
#pragma config XINST = OFF      // Extended Instruction Set Enable bit (Instruction set extension and Indexed Addressing mode disabled (Legacy mode))

// CONFIG5L
#pragma config CP0 = OFF        // Code Protection bit (Block 0 (000800-001FFFh) is not code-protected)
#pragma config CP1 = OFF        // Code Protection bit (Block 1 (002000-003FFFh) is not code-protected)
#pragma config CP2 = OFF        // Code Protection bit (Block 2 (004000-005FFFh) is not code-protected)
#pragma config CP3 = OFF        // Code Protection bit (Block 3 (006000-007FFFh) is not code-protected)

// CONFIG5H
#pragma config CPB = OFF        // Boot Block Code Protection bit (Boot block (000000-0007FFh) is not code-protected)
#pragma config CPD = OFF        // Data EEPROM Code Protection bit (Data EEPROM is not code-protected)

// CONFIG6L
#pragma config WRT0 = OFF       // Write Protection bit (Block 0 (000800-001FFFh) is not write-protected)
#pragma config WRT1 = OFF       // Write Protection bit (Block 1 (002000-003FFFh) is not write-protected)
#pragma config WRT2 = OFF       // Write Protection bit (Block 2 (004000-005FFFh) is not write-protected)
#pragma config WRT3 = OFF       // Write Protection bit (Block 3 (006000-007FFFh) is not write-protected)

// CONFIG6H
#pragma config WRTC = OFF       // Configuration Register Write Protection bit (Configuration registers (300000-3000FFh) are not write-protected)
#pragma config WRTB = OFF       // Boot Block Write Protection bit (Boot block (000000-0007FFh) is not write-protected)
#pragma config WRTD = OFF       // Data EEPROM Write Protection bit (Data EEPROM is not write-protected)

// CONFIG7L
#pragma config EBTR0 = OFF      // Table Read Protection bit (Block 0 (000800-001FFFh) is not protected from table reads executed in other blocks)
#pragma config EBTR1 = OFF      // Table Read Protection bit (Block 1 (002000-003FFFh) is not protected from table reads executed in other blocks)
#pragma config EBTR2 = OFF      // Table Read Protection bit (Block 2 (004000-005FFFh) is not protected from table reads executed in other blocks)
#pragma config EBTR3 = OFF      // Table Read Protection bit (Block 3 (006000-007FFFh) is not protected from table reads executed in other blocks)

// CONFIG7H
#pragma config EBTRB = OFF      // Boot Block Table Read Protection bit (Boot block (000000-0007FFh) is not protected from table reads executed in other blocks)

#include <xc.h>
#include "timers.h"
#include "nxlcd.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "adc.h"
#define _XTAL_FREQ 20000000 //20MHz
#define vref 5000

/*=============================| Leitura Analógica |=============================*/
void ADC_Init()
{
  ADCON0 = 0x81;               //Turn ON ADC and Clock Selection
  ADCON1 = 0x0c;               //All pins as Analog Input and setting Reference Voltages
}

unsigned int ADC_Read(unsigned char channel)
{
  if(channel > 7)              //Channel range is 0 ~ 7
    return 0;

  ADCON0 &= 0xC5;              //Clearing channel selection bits
  ADCON0 |= channel<<3;        //Setting channel selection bits
  __delay_ms(2);               //Acquisition time to charge hold capacitor
  GO_nDONE = 1;                //Initializes A/D conversion
  while(GO_nDONE);             //Waiting for conversion to complete
  return ((ADRESH<<8)+ADRESL); //Return result
}
/*===============================================================================*/

/*===============================| USANDO TIMER  |===============================*/

int segundos_que_passaram = 1;
int minutos_que_passaram = 0;
int somatorio_de_tensoes = 0;

int flag_primeira_vez = 1; // Variável que irá sinalizar se é a primeira vez que estão sendo lidas as potências

float calculo_de_potencia = 0.0; // Variável que armazena a potência atual

float vetor_de_potencias[60]; // Vetor que guardará a potência



// Interrupções do Timer

void __interrupt(high_priority) HighPriorityISR(void){
    INTCON3bits.INT1IF=0;
    //Limpa flag do INT1
    T0CONbits.TMR0ON^= 1;
    //liga desliga o TMR0
}

void __interrupt(low_priority) LowPriorityISR(void){
    // Aciona a cada 1s
    INTCONbits.TMR0IF=0;
    TMR0H=0x67;
    TMR0L = 0x6A;
    
    // Guarda a potência desse instante no array com base no instante
    vetor_de_potencias[segundos_que_passaram - 1] = calculo_de_potencia;
    
    segundos_que_passaram+=1;
    
    if(segundos_que_passaram >= 60){
        minutos_que_passaram += 1;
        segundos_que_passaram = 1;
        flag_primeira_vez = 0;
    }
}
/*===============================================================================*/


void main(void) {
    
    /* =========| CONFIGURAÇÕES TIMER |========= */
    INTCON3bits.INT1IF=0;
    INTCON2bits.INTEDG1=0;
    INTCON3bits.INT1IP=1;
    INTCON3bits.INT1IE=1;

    INTCONbits.TMR0IF=0;
    INTCON2bits.TMR0IP=0;
    INTCONbits.TMR0IE=1;

    RCONbits.IPEN=1;
    INTCONbits.GIEL=1;
    INTCONbits.GIEH=1;

    TMR0H=0x67;
    TMR0L=0x6A;

    T0CON=0b10010110; 
     
    // Inicializando posições do vetor de potências
    //for (int x = 0; x < 60; x++) {
    //    vetor_de_potencias[x] = 0.0;
    //}
    /* ========================================= */
    
    
    /* ========| CONFIGURAÇÕES DO LCD |======== */
    OpenXLCD(FOUR_BIT & LINES_5X7); // Modo 4 bits de dados e caracteres 5x7
    WriteCmdXLCD(0x01); // Limpa o LCD com retorno do cursor,
    WriteCmdXLCD(0x0c); // Sem cursor!!
    __delay_ms(2); // Atraso de 2ms para aguardar a execução do comando

    WriteCmdXLCD(0x81); // Seleciona a posição Coluna 6 e Linha 1
    putrsXLCD ("Carregando..."); // Escreve a string ?valor?
    
    __delay_ms(1000);
    
    WriteCmdXLCD(0x01); // Limpa o LCD com retorno do cursor
    __delay_ms(2); // Atraso de 2ms para aguardar a execução do comando
    
    /* ======================================== */
    
    
    /* ======| LEITURA ANALÓGICA E TELA |====== */
    Inicializa_ADC();
    
    int horas_de_consumo_por_dia = 6;
    int dis_de_consumo_por_mes = 30;
    
    int tela = 1;
        
    char calculando[16];   
    char currentWatts[10];   
    char minuteWatts[10];    
    char valorPorHora[10];    
    char valorPorMes[10];    
    
    unsigned int adcValue = 0;  
    
    float voltage = 0.0;
    float current = 0.0;
    float offsetVoltage = 2500;
    float sensitivity = 185;
    
    TRISBbits.TRISB2=1; // Botão de decremento
    TRISBbits.TRISB3=1; // Botão de incremento
    
    /* ======================================== */
    
    
    while(1) {
        /* ======| CORRENTE NESSE INSTANTE |====== */
        for (int i=0; i<20;i++)
        {
            adcValue = 0;
            adcValue  = LerADC();

            /*Convert digital value into analog voltage*/
            voltage = adcValue * ((float)vref/(float)1024);  

            //if(voltage >= 2500)
                current += ((voltage - offsetVoltage)/sensitivity);
            
            //else if(voltage <= 2500)
              //  current += ((offsetVoltage-voltage)/sensitivity);
        }
        current/=20;        
        /* ======================================= */
        

        /* ======| POTÊNCIA NESSE INSTANTE |====== */
        calculo_de_potencia = (110 * current)/1000;
        
        // Sanitizando dado da potência no instante para exibí-la adequadamente
        if (calculo_de_potencia < 0.0) {
            sprintf(currentWatts,"%.2f",0.0);   
        } else {
            sprintf(currentWatts,"%.2f",calculo_de_potencia);   
        }
        strcat(currentWatts," kW");	// Concatenando unidade de medida (kW)      
        /* ======================================= */
        
        
        /* ========| POTÊNCIA POR MINUTO |======== */
        float potencia_por_minuto = 0.0;
        for (int y = 0; y<60;y++) {
            potencia_por_minuto += vetor_de_potencias[y];
        }
        if (potencia_por_minuto < 0.0) {
            sprintf(minuteWatts,"%.2f",0.0);   
        } else {
            sprintf(minuteWatts,"%.2f",potencia_por_minuto/60);   
        }      
        strcat(minuteWatts," kWh");	// Concatenando unidade de medida (kWh)   
        /* ======================================= */
        
        
        /* ========| PREÇO POR MINUTO E POR HORA |======== */
        float preco_em_reais = 0.0;
        preco_em_reais = potencia_por_minuto * (0.92/60.0);
        if (potencia_por_minuto < 0.0) {
            sprintf(valorPorHora,"R$ %.2f",0.0);   
        } else {
            sprintf(valorPorHora,"R$ %.2f",preco_em_reais);   
        }    
        if (potencia_por_minuto < 0.0) {
            sprintf(valorPorMes,"R$ %.2f",0.0);   
        } else {
            sprintf(valorPorMes,"R$ %.2f",preco_em_reais*horas_de_consumo_por_dia*dis_de_consumo_por_mes);   
        }    
        /* =============================================== */
        
                    
        /* =======| COMPORTAMENTO DA TELA |======= */
        WriteCmdXLCD(0x01); // Limpa o LCD com retorno do cursor
        __delay_ms(2); // Atraso de 2ms para aguardar a execução do comando
        // Retorna à tela anterior
        
        if (!PORTBbits.RB2) {
            if (tela > 1) {
                tela--;
                __delay_ms(500);
            }
        }
        
        // Avança para a tela seguinte
        if (!PORTBbits.RB3) {
            if (tela < 3) {
                tela++;
                __delay_ms(500);
            }
        }
        
        switch(tela) {
            case 1:
                WriteCmdXLCD(0x81);
                putrsXLCD ("Gasto por hora");    
                if (flag_primeira_vez) {
                    WriteCmdXLCD(0xC0); 
                    sprintf(calculando,"Aguarde: (%d/60)",segundos_que_passaram);   
                    putrsXLCD (calculando);
                } else { 
                    WriteCmdXLCD(0xC5); 
                    putrsXLCD (valorPorHora);
                }
                break;
            case 2:
                WriteCmdXLCD(0x80);
                putrsXLCD ("Previsao no mes");       
                if (flag_primeira_vez) {
                    WriteCmdXLCD(0xC0); 
                    sprintf(calculando,"Aguarde: (%d/60)",segundos_que_passaram);   
                    putrsXLCD (calculando);
                } else { 
                    WriteCmdXLCD(0xC5); 
                    putrsXLCD (valorPorMes);
                }
                break;
            case 3:
                WriteCmdXLCD(0x84);
                putrsXLCD ("Creditos");        
                WriteCmdXLCD(0xC4); 
                putrsXLCD ("Equipe 6");
                break;
            default:
                WriteCmdXLCD(0xC6); 
                putrsXLCD ("?");
                break;
        }
        
         __delay_ms(100); 
        /* ======================================= */
    }
}