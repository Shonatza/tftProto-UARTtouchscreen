/*
 * File:   main.c
 * Author: felipe.martinez.lassalle
 *
 */

#include "config.h"


unsigned int x, y;
char checksum;
char debug_buff[4];
int i, j;

void __attribute__ ((interrupt,auto_psv)) _ADC1Interrupt(void) {

   x = ADC1BUF0;
   read_update(1);              // 0:x 1:y
   //sleep
   for(i=0; i < 10; i++)for(j=0; j<1598; j++);
   y = ADC1BUF0;
   read_update(0);              // 0:x 1:y
   
   if(x > 25 && y > 25) {      // Only compute if pressed
    UART1_write('X');          // Init msg
    UART1_write('Y');

    // FAT MSG
    // X
    UART1_write((x >> 8) & 0x3); // h
    UART1_write(x & 0xFF);       // l

    // Y
    UART1_write((y >> 8) & 0x3); // h
    UART1_write(y & 0xFF);       // l   
    checksum = ((x >> 8) & 0x3) ^ ((y >> 8) & 0x3);
    UART1_write(checksum);
    UART1_write('\n');      // Footer byte

    // DEBUG MSG
    // X
    /*for(i = 0; i < 4; ++i) {
        debug_buff[i] = (x%10) + '0';
        x /= 10;
    }
    for(i = 3; i >= 0; --i) UART1_write(debug_buff[i]);

     // Y
    for(i = 0; i < 4; ++i) {
        debug_buff[i] = (y%10) + '0';
        y /= 10;
    }
    for(i = 3; i >= 0; --i) UART1_write(debug_buff[i]);

    UART1_write('\n');
    //sleep
    for(i=0; i < 50; i++)for(j=0; j<1598; j++);*/
   }

   IFS0bits.AD1IF = 0;          // Clear ADC interrupt flag
}

void UART1_write(char data) {
   while(U1STAbits.UTXBF);
   U1TXREG = data;
}

void read_update(int read_mode) {

   if(read_mode == 0) {
      AD1PCFG = 0xFFF7;       // Just in case, put every pin as digital ones
      TRISAbits.TRISA0 = 0;   // X+
      TRISAbits.TRISA1 = 0;   // X-
      LATAbits.LATA0 = 1;     // X+
      LATAbits.LATA1 = 0;     // X-

      TRISBbits.TRISB0 = 1;   // Y+
      TRISBbits.TRISB1 = 1;   // Y-
      AD1CHS  = 0x0003;       // Connect RB1/AN3 as CH0 input 
   }
   else {

      AD1PCFG = 0xFFFD;       // Just in cases, put every pin as digital ones
      TRISBbits.TRISB0 = 0;   // Y+
      TRISBbits.TRISB1 = 0;   // Y-
      LATBbits.LATB0 = 1;     // Y+
      LATBbits.LATB1 = 0;     // Y-

      TRISAbits.TRISA0 = 1;   // X+
      TRISAbits.TRISA1 = 1;   // X-
      
      AD1CHS  = 0x0001;       // Connect RA1/AN1 as CH0 input

   }
}

int main (void) {  

   read_update(0);        // 0: y 1: x
   CNPU1bits.CN2PUE = 0;  // Disable pull-up 
   CNPD1bits.CN2PDE = 1;   // Enable pull-down
   CNPU1bits.CN3PUE = 0;  // Disable pull-up 
   CNPD1bits.CN3PDE = 1;  // Enable pull-down
   CNPU1bits.CN4PUE = 0;  // Disable pull-up 
   CNPD1bits.CN4PDE = 1;  // Enable pull-down
   CNPU1bits.CN5PUE = 0;  // Disable pull-up 
   CNPD1bits.CN5PDE = 1;  // Enable pull-down

   AD1CON1bits.ADON   = 0; // Disable ADC
   AD1CON1bits.ADSIDL = 0; // Continue module operation in Idle mode
   AD1CON1bits.FORM   = 0; // Integer
   AD1CON1bits.SSRC   = 7; // Internal counter ends sampling and starts converting (auto-convert)
   AD1CON1bits.ASAM   = 0; // Sampling begins when SAMP bit is set
   AD1CON1bits.SAMP   = 0; // A/D sample/hold amplifier is holding

   AD1CON2bits.VCFG   = 0; // Reference voltage is AVdd, AVss
   AD1CON2bits.OFFCAL = 0; // Converts to get the actual input value
   AD1CON2bits.CSCNA  = 0; // Do not scan inputs
   AD1CON2bits.SMPI   = 0; // Interrupts at the completion for each sample/convert sequence

   AD1CON3bits.ADRC =  0;  // Clock derived from system clock
   AD1CON3bits.SAMC = 31;  // Sample time = 31 Tad
   AD1CON3bits.ADCS =  0;  // Tad = 2*Tcy

   IPC3bits.AD1IP = 7;     // Interrupt priority
   IFS0bits.AD1IF = 0;     // Clear ADC interrupt flag
   IEC0bits.AD1IE = 1;     // Enable ADC interrupts

   AD1CON1bits.ADON = 1;   // Turn ADC on
   AD1CON1bits.ASAM = 1;   // Start sampling

   //Baud Rate
   U1MODEbits.BRGH = 0;
   U1BRG = 25;             // Baud Rate configured to 9600bps

   //Mode Configuration
   U1MODEbits.PDSEL = 0;   // 00 -> 8-bit data, no parity
   U1MODEbits.STSEL = 0;   // 0 -> 1 Stop bit

   //Enable UART
   U1MODEbits.UARTEN = 1;  // Enable UART
   //Enable Tx
   U1STAbits.UTXEN = 1;    // Enable Transmitter

   while(1){}              // Interruption based

}
