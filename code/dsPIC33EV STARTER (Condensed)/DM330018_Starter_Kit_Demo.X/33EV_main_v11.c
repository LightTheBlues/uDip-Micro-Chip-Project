/**********************************************************************
 * � 2014 Microchip Technology Inc.
 *
 * FileName:        33EV_main_v11.c
 * Version:         1.1 (added UART) THIS IS THE PRODUCTION RELEASE CODE
 * Dependencies:    no header file is used
 * Processor:       dsPIC33EV256GM106
 * Compiler:        MPLAB� XC16 V1.23 or higher (dsPIC33EV support required)
 *
 * SOFTWARE LICENSE AGREEMENT:
 * Microchip Technology Incorporated ("Microchip") retains all ownership and
 * intellectual property rights in the code accompanying this message and in all
 * derivatives hereto.  You may use this code, and any derivatives created by
 * any person or entity by or on your behalf, exclusively with Microchip's
 * proprietary products.  Your acceptance and/or use of this code constitutes
 * agreement to the terms and conditions of this notice.
 *
 * CODE ACCOMPANYING THIS MESSAGE IS SUPPLIED BY MICROCHIP "AS IS".  NO
 * WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED
 * TO, IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE APPLY TO THIS CODE, ITS INTERACTION WITH MICROCHIP'S
 * PRODUCTS, COMBINATION WITH ANY OTHER PRODUCTS, OR USE IN ANY APPLICATION.
 *
 * YOU ACKNOWLEDGE AND AGREE THAT, IN NO EVENT, SHALL MICROCHIP BE LIABLE, WHETHER
 * IN CONTRACT, WARRANTY, TORT (INCLUDING NEGLIGENCE OR BREACH OF STATUTORY DUTY),
 * STRICT LIABILITY, INDEMNITY, CONTRIBUTION, OR OTHERWISE, FOR ANY INDIRECT, SPECIAL,
 * PUNITIVE, EXEMPLARY, INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, FOR COST OR EXPENSE OF
 * ANY KIND WHATSOEVER RELATED TO THE CODE, HOWSOEVER CAUSED, EVEN IF MICROCHIP HAS BEEN
 * ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE.  TO THE FULLEST EXTENT
 * ALLOWABLE BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN ANY WAY RELATED TO
 * THIS CODE, SHALL NOT EXCEED THE PRICE YOU PAID DIRECTLY TO MICROCHIP SPECIFICALLY TO
 * HAVE THIS CODE DEVELOPED.
 *
 * You agree that you are solely responsible for testing the code and
 * determining its suitability.  Microchip has no obligation to modify, test,
 * certify, or support the code.
 *
 * REVISION HISTORY:
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Author                Date      		Comments on this revision
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Paul Schreiber	  10/02/2014        V1.1 release (added UART teminal using external ADM00393)
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * ADDITIONAL NOTES:
 * Software designed to be loaded to the dsPIC33EV256GM106 Starter Kit
 *
 * What the demo does:
 * a) CAN, LIN and SENT are all available and switch, pot, and temperature data transmitted out all 3
 * b) The active HW interface transmits every 1sec the pot voltage, switch status and the temp in C
 * c) LEDs active when associated switch is pressed in Transmitter mode
 * d) CAN is setup for 250kbps speed with 10 Tq per bit
 * e) LIN baudrate is 4800, and the analyzer timeout should be set to 1000ms
 * f) The LIN interface requires application of external +12VDC to the BAT terminal.
 *
 * The default POR state is board is in TRANSMIT mode. This is indicated by the 3 LEDs flashing 1-2-3-2-1.
 * If any of the 3 user switches are depressed longer than 100ms while MCLR is pressed,
 * this places the board into RECEIVE mode. All 3 LEDs blink at once 5 times.
 * User must release switch(s) to begin RECEIVE mode.
 *
 * There is a UART monitor for both transmit and receive modes. It will dump ASCII data to a terminal.
 *
 * Connecting an external ADM00393 board as follows will allow message monitoring for all interfaces.
 *
 * Be SURE the ADM00393 has the power select jumper set to 5V! There are 3 jumper wires needed.
 *
 * TXD - RB4 goes to the RXD pin of the ADM
 * GND - GND goes to the G pin of the ADM
 * DVDD - DVDD goes to the VDD pin of the ADM
 *
 * The demo baud rate is set to 38,400/8/N/1. This can be changed by re-compiling with different
 * BAUDxxxx settings. Be sure your terminal program is also set to this configuration.
 *
 * The CAN message code is based on App Note AN1249. See it for details on setting up the CAN.
 *
 * IMPORTANT!!
 *
 * In order for this demo to run, CAN must be active. This requires either a CAN bus, a CAN analyzer or another
 * DM330018 board to be connected to the CAN interface. Otherwise, the code will 'hang', waiting for a CAN message.
 * If CAN is not needed (say you want to do SENT only), then you must comment out a line of code and recompile.
 * See the User's Guide for instructions.
 *
 *
 * The Processor starts up with the crystal + PLL, no clock switching!
 *************************************************************************************************/
#include <p33EV256GM106.h>
//
// includes for the header files
//
// port definitions for the starter kit
//
#define         LED1        _LATC4
#define         TRISLED1    _TRISC4
#define         LED2        _LATC5
#define         TRISLED2    _TRISC5
#define         LED3        _LATC6
#define         TRISLED3    _TRISC6
#define         TRIS_POT    _TRISG6
#define         TRIS_TEMP   _TRISG7
#define         ANSEL_POT   _ANSG6
#define         ANSEL_TEMP  _ANSG7
#define         SW1         _RC7
#define         SW2         _RC8
#define         SW3         _RC9
#define         FCAN        40000000                // Fcyc = 1/2Fpll
#define         BAUD9600    ((FCAN/9600)/16) - 1
#define         BAUD19200   ((FCAN/19200)/16) - 1
#define         BAUD38400   ((FCAN/38400)/16) - 1   // this is what the demo UART serial baud rate is
#define         BAUD576000  ((FCAN/57600)/16) - 1   // selection of transmitter baud rate divisors
#define         ANSEL_RTS   _ANSE12
#define         ANSEL_CTS   _ANSE13
#define         TRIS_RTS    _TRISE12
#define         TRIS_MON    _TRISB4
#define         TRANSMIT 1
#define         RECEIVE 0

/* CAN filter and mask defines */
/* Macro used to write filter/mask ID to Register CiRXMxSID and
CiRXFxSID. For example to setup the filter to accept a value of
0x123, the macro when called as CAN_FILTERMASK2REG_SID(0x123) will
write the register space to accept message with ID 0x123
USE FOR STANDARD MESSAGES ONLY */
#define CAN_FILTERMASK2REG_SID(x) ((x & 0x07FF)<< 5)
/* the Macro will set the "MIDE" bit in CiRXMxSID */
#define CAN_SETMIDE(sid) (sid | 0x0008)
/* the macro will set the EXIDE bit in the CiRXFxSID to
accept extended messages only */
#define CAN_FILTERXTD(sid) (sid | 0x0008)
/* the macro will clear the EXIDE bit in the CiRXFxSID to
accept standard messages only */
#define CAN_FILTERSTD(sid) (sid & 0xFFF7)

//  Macros for Configuration Fuse Registers 
_FOSCSEL(FNOSC_PRIPLL);
_FOSC(FCKSM_CSDCMD & OSCIOFNC_OFF & POSCMD_XT);
// Startup directly into XT + PLL
// OSC2 Pin Function: OSC2 is Clock Output
// Primary Oscillator Mode: XT Crystal

_FWDT(FWDTEN_OFF);      // Watchdog Timer Enabled/disabled by user software

_FICD(ICS_PGD2);        // PGD3 for external PK3/ICD3/RealIce, use PGD2 for PKOB
_FPOR(BOREN0_OFF);      // no brownout detect
_FDMT(DMTEN_DISABLE);   // no deadman timer  <<< *** New feature, important to DISABLE

#define NUM_OF_ECAN_BUFFERS 32
#define MSG_SID 0x123              // the arbitrary CAN SID of the transmitted message

/* ECAN message type identifiers */
#define CAN_MSG_DATA 0x01
#define CAN_MSG_RTR 0x02
#define CAN_FRAME_EXT 0x03
#define CAN_FRAME_STD 0x04
#define CAN_BUF_FULL 0x05
#define CAN_BUF_EMPTY 0x06

#define NUM_DIGITS 5               // floating point digits to print
#define STRING_BUFFER_SIZE 64      // arbitrary length message buffer

volatile unsigned int ecan1MsgBuf[NUM_OF_ECAN_BUFFERS][8]
__attribute__((aligned(NUM_OF_ECAN_BUFFERS * 16)));

/* CAN receive message structure in RAM */
typedef struct{
	/* keep track of the buffer status */
	unsigned char buffer_status;
	/* RTR message or data message */
	unsigned char message_type;
	/* frame type extended or standard */
	unsigned char frame_type;
	/* buffer being used to send and receive messages */
	unsigned char buffer;
	/* 29 bit id max of 0x1FFF FFFF
	*  11 bit id max of 0x7FF */
	unsigned long id;
	unsigned int data[8];
	unsigned char data_length;
}mID;

// Prototype Declarations
void rxECAN(mID *message);
void clearRxFlags(unsigned char buffer_number);
void oscConfig(void);
void clearIntrflags(void);
void ecan1WriteMessage(void);
void init_hw(void);
void delay_10ms(unsigned char num);
void Delayus(int);
void Test_Mode(void);
void LED_Transmit(void);
void LED_Receive(void);
void ADCInit(void);
void ADCConvert(int);
void InitMonitor(void);
void Calc_Checksum(int);
void InitCAN(void);
void CAN_Transmit(void);
void Transmit_Data(void);
void Receive_Data(void);
void ftoa(float, char*);
void Can_RX_to_UART(void);

// send a character to the serial port
void putU2(int);
void putsU2(char*);

volatile int channel, PotValue, TempValue, AverageValue, i;
volatile int f_tick, s_tick, p0, p1, id_byte, data_byte, checksum, lin_index, lin_start;
volatile int tickTime = 50;             // Tick time in us
volatile float peripheralClk = 39.77;   // in Mhz
volatile float Pot_Volts;
volatile char can_rx, sent_rx, lin_rx;  // receive message flags

char Buf_result[NUM_DIGITS + 2];        // digits + '.' and allow for '-'
char *pBuf;                             // buffer for ASCII result of a float
char s[STRING_BUFFER_SIZE];             // s[] holds a string to transmit
unsigned char mode;
unsigned int ascii_lo, ascii_hi, hex_dig;

volatile int datal;
volatile int datah;

mID canRxMessage;

int main(void)
{

    // Configure Oscillator Clock Source
    oscConfig();

    // Clear Interrupt Flags
    clearIntrflags();

    // Initialize hardware on the board
    init_hw();

    // Initialize the monitor UART2 module
    InitMonitor();

    // Test to see if we are in TRANSMIT or RECIEVE mode for the demo, show LEDs
    Test_Mode();

    if (mode == TRANSMIT)
    {
        LED_Transmit();
    }
    else
    {
        LED_Receive();
        while ((SW1 == 0) | (SW2 == 0) | (SW3 == 0)); // wait to release all keys
        LED1 = 0; // initialize LEDs to all off
        LED2 = 0;
        LED3 = 0;
    }
    // Initialize the ADC converter
    ADCInit();

    // Initialize the CAN module
        InitCAN();

    // main loop: every 4 Timer1 ticks (1sec), scan the sensors and transmit the data
    // or wait for a Receive interrupt from 1 of the 3 interfaces
    //
    s_tick = 0;
    while (1)
    {
        if (mode == RECEIVE)
        {
     /* check to see when a message is received and move the message
		into RAM and parse the message */
		if(canRxMessage.buffer_status == CAN_BUF_FULL)
		{
			rxECAN(&canRxMessage);

			/* reset the flag when done */
			canRxMessage.buffer_status = CAN_BUF_EMPTY;
        }
       
            Receive_Data();
            s_tick = 0;
        }
        else if (mode == TRANSMIT)
        {
 //
 // wait for the 250ms timer. User can accumulate s_ticks to get longer delays
 //
            while (s_tick <= 3);        // wait for 1 second
            s_tick = 0;                 // clear flag
            Transmit_Data();

        }
    }
}
//
// Transmit Mode sequence
//

void Transmit_Data(void)
{
    //
    // The LEDs reflect the switch status
    //
    LED1 = ~SW1;
    LED2 = ~SW2;
    LED3 = ~SW3;

    //
    // read the pot value and save it
    //
    ADCConvert(19);
    PotValue = AverageValue;
    Delayus(100);
    Pot_Volts = (float)(PotValue * (float)5.0 / (float)4096.0);
    //
    // convert to ASCII
    //
    pBuf = Buf_result;
    ftoa(Pot_Volts, pBuf);
    
    U2STAbits.UTXEN = 1;
    putsU2("***TRANSMITTING ON-BOARD SENSOR VALUES***");
    while (U2STAbits.TRMT == 0);
    U2TXREG = 0x0a;
    while (U2STAbits.TRMT == 0);
    U2TXREG = 0x0d;
     
    putsU2("Local Pot Voltage: Reading = ");

    for (i = 0; i <= (NUM_DIGITS - 1); i++)
    {
        while (U2STAbits.TRMT == 0);
        U2TXREG = Buf_result[i];
    }
    while (U2STAbits.TRMT == 0);
    U2TXREG = 0x0a;
    while (U2STAbits.TRMT == 0);
    U2TXREG = 0x0d;

    //
    // read temperature sensor and save it
    //
    ADCConvert(18);
    TempValue = AverageValue;
    Delayus(100);

    //
    // test print the temperature reading out the UART
    //
    Pot_Volts = (float)((TempValue - 368) / 15.974);
    //
    // convert to ASCII
    //
    pBuf = Buf_result;
    ftoa(Pot_Volts, pBuf);

    U2STAbits.UTXEN = 1;

    putsU2("Local Temperature: Reading = ");

    for (i = 0; i <= (NUM_DIGITS - 1); i++)
    {
        while (U2STAbits.TRMT == 0);
        U2TXREG = Buf_result[i];
    }
    while (U2STAbits.TRMT == 0);
    U2TXREG = 0x0a;
    while (U2STAbits.TRMT == 0);
    U2TXREG = 0x0d;
    //
    // test print the 3 switch statuses
    //

    putsU2("Local Switch status");
    while (U2STAbits.TRMT == 0);
    U2TXREG = 0x0a;
    while (U2STAbits.TRMT == 0);
    U2TXREG = 0x0d;
    // ON = pressed  OFF = up
    if (SW1)
    {
        putsU2("SW1: OFF ");
    }
    else
    {
        putsU2("SW1: ON");
    }
    while (U2STAbits.TRMT == 0);
    U2TXREG = 0x0a;
    while (U2STAbits.TRMT == 0);
    U2TXREG = 0x0d;

    // ON = pressed  OFF = up
    if (SW2)
    {
        putsU2("SW2: OFF ");
    }
    else
    {
        putsU2("SW2: ON");
    }
    while (U2STAbits.TRMT == 0);
    U2TXREG = 0x0a;
    while (U2STAbits.TRMT == 0);
    U2TXREG = 0x0d;

    // ON = pressed  OFF = up
    if (SW3)
    {
        putsU2("SW3: OFF ");
    }
    else
    {
        putsU2("SW3: ON");
    }
    while (U2STAbits.TRMT == 0);
    U2TXREG = 0x0a;
    while (U2STAbits.TRMT == 0);
    U2TXREG = 0x0d;
    // format and send out the CAN port
    //
    // In order for the demo to run, the CAN controller needs an ACK signal
    // If you desire to run the demo for SENT/LIN only, then comment out the
    // following line of code and recompile
    
    CAN_Transmit();     // Transmit CAN. COMMENT OUT FOR LIN/SENT ONLY!!
    Delayus(5000);

}

void Receive_Data(void)
{
    //
    // have we received any messages from somewhere?
    //
    if (can_rx == 1)
    {
        Can_RX_to_UART();
        can_rx = 0;
    }
}
void clearIntrflags(void)
{
    /* Clear Interrupt Flags */

    IFS0 = 0;
    IFS1 = 0;
    IFS2 = 0;
    IFS3 = 0;
    IFS4 = 0;
    //IPC16bits.U1EIP = 6;        //service the LIN framing error before the RX
    IPC2bits.U1RXIP = 4;
}
void Can_RX_to_UART(void)
{
    // CAN message out the monitor UART
    //
    U2STAbits.UTXEN = 1;
    putsU2("*** REMOTE CAN MESSAGE ID = 0x");
   //
    // display remote ID byte
    //
    hex_dig = (char)((canRxMessage.id & 0xff00) >> 8);   // upper byte
    ascii_hi = hex_dig & 0xF0;                  // Obtain the upper 4 bits (MSBs) of hex number
    ascii_hi = (ascii_hi >> 4) + 0x30;          // ASCII conversion
    ascii_lo = (hex_dig & 0x0F) + 0x30;         // Obtain the lower 4 bits (LSBs) of hex number

    putU2(ascii_hi);
    putU2(ascii_lo);                    // send out the upper ID byte as ASCII

    hex_dig = (char)(canRxMessage.id & 0x00ff); // lower byte
    ascii_hi = hex_dig & 0xF0;                  // Obtain the upper 4 bits (MSBs) of hex number
    ascii_hi = (ascii_hi >> 4) + 0x30;          // ASCII conversion
    ascii_lo = (hex_dig & 0x0F) + 0x30;         // Obtain the lower 4 bits (LSBs) of hex number

    putU2(ascii_hi);
    putU2(ascii_lo);                    // send out the lower ID byte as ASCII

    putsU2(" RECEIVED ***");
    while (U2STAbits.TRMT == 0);
    U2TXREG = 0x0a;
    while (U2STAbits.TRMT == 0);
    U2TXREG = 0x0d;
    putsU2("Remote Pot Voltage: ");

    PotValue = ((canRxMessage.data[4]) | (canRxMessage.data[5] << 8));
    Pot_Volts = (float)(PotValue * (float)5.0 / (float)4096.0);
    //
    // convert to ASCII
    //
    pBuf = Buf_result;
    ftoa(Pot_Volts, pBuf);
    for (i = 0; i <= (NUM_DIGITS - 1); i++)
    {
        while (U2STAbits.TRMT == 0);
        U2TXREG = Buf_result[i];
    }
    while (U2STAbits.TRMT == 0);
    U2TXREG = 0x0a;
    while (U2STAbits.TRMT == 0);
    U2TXREG = 0x0d;

    putsU2("Remote Temperature: ");

    //
    //  print the temperature reading out the UART
    //
    datal = ((canRxMessage.data[2]) | (canRxMessage.data[3] << 8));
    Pot_Volts = (float)((datal - 368) / 15.974);
    //
    // convert to ASCII
    //
    pBuf = Buf_result;
    ftoa(Pot_Volts, pBuf);
    for (i = 0; i <= (NUM_DIGITS - 1); i++)
    {
        while (U2STAbits.TRMT == 0);
        U2TXREG = Buf_result[i];
    }
    while (U2STAbits.TRMT == 0);
    U2TXREG = 0x0a;
    while (U2STAbits.TRMT == 0);
    U2TXREG = 0x0d;

    putsU2("Remote Switch Status");

    while (U2STAbits.TRMT == 0);
    U2TXREG = 0x0a;
    while (U2STAbits.TRMT == 0);
    U2TXREG = 0x0d;
    // ON = pressed  OFF = up
    if ((canRxMessage.data[0] & 0x4) == 0)
    {
        putsU2("SW3: OFF ");
    }
    else
    {
        putsU2("SW3: ON");
    }
    while (U2STAbits.TRMT == 0);
    U2TXREG = 0x0a;
    while (U2STAbits.TRMT == 0);
    U2TXREG = 0x0d;

    // ON = pressed  OFF = up
    if ((canRxMessage.data[0] & 0x2) == 0)
    {
        putsU2("SW2: OFF ");
    }
    else
    {
        putsU2("SW2: ON");
    }
    while (U2STAbits.TRMT == 0);
    U2TXREG = 0x0a;
    while (U2STAbits.TRMT == 0);
    U2TXREG = 0x0d;

    // ON = pressed  OFF = up
    if ((canRxMessage.data[0] & 0x1) == 0)
    {
        putsU2("SW1: OFF ");
    }
    else
    {
        putsU2("SW1: ON");
    }

    while (U2STAbits.TRMT == 0);
    U2TXREG = 0x0a;
    while (U2STAbits.TRMT == 0);
    U2TXREG = 0x0d;
    //
    // toggle LED 1 to show CAN message reveived
    //
    LED1 = 1;
    //
    // wait 100ms, turn off LED1
    //
    delay_10ms(10);
    LED1 = 0;
}

void init_hw(void)
{
    int j;

    // set up the LED and switch ports

    TRISLED1 = 0;
    TRISLED2 = 0;
    TRISLED3 = 0;
    ANSEL_POT = 1;
    ANSEL_TEMP = 1;
    TRIS_POT = 1;
    TRIS_TEMP = 1;
    ANSELC = ANSELC & 0xFC3F;   // (re)set the 3 switch bits + CAN due to error in v1.20 header
    s_tick = 0;
    f_tick = 0;                 // the timer ticks


    //
    // Timer 1 to generate an interrupt every 250ms
    //
    T1CONbits.TON = 0;          // Disable Timer1
    T1CONbits.TCS = 0;          // Select internal instruction cycle clock
    T1CONbits.TGATE = 0;        // Disable Gated Timer mode
    T1CONbits.TCKPS = 0x3;      // Select 1:256 Prescaler
    PR1 = 39062;                // Load the period value (250ms/(256*25ns))
    IPC0bits.T1IP = 0x03;       // Set Timer 1 Interrupt Priority Level
    IFS0bits.T1IF = 0;          // Clear Timer 1 Interrupt Flag
    IEC0bits.T1IE = 1;          // Enable Timer1 interrupt

    //
    // Timer 2 to generate an interrupt every 10ms
    //
    T2CONbits.TON = 0;          // Disable Timer2
    T2CONbits.TCS = 0;          // Select internal instruction cycle clock
    T2CONbits.TGATE = 0;        // Disable Gated Timer mode
    T2CONbits.TCKPS = 0x3;      // Select 1:256 Prescaler
    TMR2 = 0x00;                // Clear timer register
    PR2 = 1562;                 // Load the period value (10ms/(256*25ns))
    IPC1bits.T2IP = 0x02;       // Set Timer 2 Interrupt Priority Level
    IFS0bits.T2IF = 0;          // Clear Timer 2 Interrupt Flag
    IEC0bits.T2IE = 1;          // Enable Timer2 interrupt

    T2CONbits.TON = 1;          // Start Timer2
    T1CONbits.TON = 1;          // Start Timer1
}

void Test_Mode(void)
{
    if ((SW1 == 0) || (SW2 == 0) || (SW3 == 0))
    {
        delay_10ms(30); // wait 300ms

        if ((SW1 == 0) || (SW2 == 0) || (SW3 == 0))
        {
            mode = RECEIVE;
            return;
        }
    }

    mode = TRANSMIT;
}

void LED_Receive(void)
{
    for (i = 0; i <= 5; i++) // fast blink all LEDs
    {
        LED1 = 0;
        LED2 = 0;
        LED3 = 0;
        delay_10ms(15);
        LED1 = 1;
        LED2 = 1;
        LED3 = 1;
        delay_10ms(12);
    }
}

void LED_Transmit(void)
{
    //
    // sequence LEDs on & off as a power-up test
    //
    LED1 = 1;
    LED2 = 0;
    LED3 = 0;
    //
    // wait 200ms, turn on LED2
    //
    delay_10ms(20);
    LED2 = 1;
    //
    // wait 200ms, turn on LED3
    //
    delay_10ms(20);
    LED3 = 1;
    //
    // wait 500ms, turn off LED3
    //
    delay_10ms(50);
    LED3 = 0;
    //
    // wait 200ms, turn off LED2
    //
    delay_10ms(20);
    LED2 = 0;
    //
    // wait 200ms, turn off LED1
    //
    delay_10ms(20);
    LED1 = 0;
}

void oscConfig(void)
{

    //  Configure Oscillator to operate the device at 80MHz/40MIPs
    // 	Fosc= Fin*M/(N1*N2), Fcy=Fosc/2
    // 	Fosc= 8M*40/(2*2)=80Mhz for 8M input clock
    // To be safe, always load divisors before feedback
    
   
    CLKDIVbits.PLLPOST = 0;     // N1=2
    CLKDIVbits.PLLPRE = 0;      // N2=2
    PLLFBD = 38;                // M=(40-2), Fcyc = 40MHz for ECAN baud timer


    // Disable Watch Dog Timer

    RCONbits.SWDTEN = 0;

}

void InitCAN(void)
{
    //
    // drive the CAN STANDBY driver pin low
    //
    _TRISG9 = 0;
    _LATG9 = 0;
    _TRISF1 = 0;
    _TRISF0 = 1;

    //
    // remap the CAN module to the proper pins on the board
    //
    RPINR26 = 0x60;         // connect CAN RX to RPI96
    RPOR9 = 0x000E;         // connect CAN TX to RP97

    C1CTRL1bits.REQOP = 4;

    while (C1CTRL1bits.OPMODE != 4);
    C1CTRL1bits.WIN = 0;

    /* Set up the CAN module for 250kbps speed with 10 Tq per bit. */

    C1CFG1 = 0x47;          // BRP = 8 SJW = 2 Tq
    C1CFG2 = 0x2D2;
    C1FCTRL = 0xC01F;       // No FIFO, 32 Buffers

    //
    // set up the CAN DMA0 for the Transmit Buffer
    //
    DMA0CONbits.SIZE = 0x0;
    DMA0CONbits.DIR = 0x1;
    DMA0CONbits.AMODE = 0x2;
    DMA0CONbits.MODE = 0x0;
    DMA0REQ = 70;
    DMA0CNT = 7;
    DMA0PAD = (volatile unsigned int)&C1TXD;
    DMA0STAL = (unsigned int)&ecan1MsgBuf;
    DMA0STAH = (unsigned int)&ecan1MsgBuf;

    C1TR01CONbits.TXEN0 = 0x1;          // Buffer 0 is the Transmit Buffer
    C1TR01CONbits.TX0PRI = 0x3;         // transmit buffer priority

    DMA0CONbits.CHEN = 0x1;

    /* initialise the DMA channel 2 for ECAN Rx */
;
    /* setup channel 2 for peripheral indirect addressing mode
    normal operation, word operation and select as Rx to peripheral */
    DMA2CON = 0x0020;
    /* setup the address of the peripheral ECAN1 (C1RXD) */
	DMA2PAD = (volatile unsigned int)&C1RXD;
 	/* Set the data block transfer size of 8 */
 	DMA2CNT = 7;
 	/* automatic DMA Rx initiation by DMA request */
	DMA2REQ = 0x0022;
	/* start adddress offset value */
	DMA2STAL=(unsigned int)(&ecan1MsgBuf);
    DMA2STAH=(unsigned int)(&ecan1MsgBuf);
	/* enable the channel */
	DMA2CONbits.CHEN=1;

	/* 4 CAN Messages to be buffered in DMA RAM */
	C1FCTRLbits.DMABS=0b000;

    /* Filter configuration */
	/* enable window to access the filter configuration registers */
	C1CTRL1bits.WIN = 0b1;
	/* select acceptance mask 0 filter 0 buffer 1 */
	C1FMSKSEL1bits.F0MSK = 0;

    /* setup the mask to check every bit of the standard message, the macro when called as */
    /* CAN_FILTERMASK2REG_SID(0x7FF) will write the register C1RXM0SID to include every bit in */
    /* filter comparison */
    C1RXM0SID=CAN_FILTERMASK2REG_SID(0x7FF);

	/* configure accpetence filter 0
	setup the filter to accept a standard id of 0x123,
	the macro when called as CAN_FILTERMASK2REG_SID(0x123) will
	write the register C1RXF0SID to accept only standard id of 0x123
	*/
	C1RXF0SID = CAN_FILTERMASK2REG_SID(MSG_SID);
	/* set filter to check for standard ID and accept standard id only */
	C1RXM0SID = CAN_SETMIDE(C1RXM0SID);
	C1RXF0SID = CAN_FILTERSTD(C1RXF0SID);
	/* acceptance filter to use buffer 1 for incoming messages */
	C1BUFPNT1bits.F0BP = 0b0001;
	/* enable filter 0 */
	C1FEN1bits.FLTEN0 = 1;
    /* clear window bit to access ECAN control registers */
	C1CTRL1bits.WIN = 0;

    /* ECAN1, Buffer 1 is a Receive Buffer */
	C1TR01CONbits.TXEN1 = 0;

    /* clear the buffer and overflow flags */
	C1RXFUL1=C1RXFUL2=C1RXOVF1=C1RXOVF2=0x0000;

    // Place the ECAN module in Normal mode.
    C1CTRL1bits.REQOP = 0;
    while (C1CTRL1bits.OPMODE != 0);

    //
    // CAN RX interrupt enable - 'double arm' since 2-level nested interrupt
    //
    C1INTEbits.RBIE = 1;
    IEC2bits.C1IE = 1;
}

void CAN_Transmit(void)
{
    ecan1MsgBuf[0][0] = MSG_SID << 2;

    ecan1MsgBuf[0][1] = 0x0000;
    /* CiTRBnDLC = 0b0000 0000 xxx0 1111
    EID<17:6> = 0b000000
    RTR = 0b0
    RB1 = 0b0
    RB0 = 0b0
    DLC = 6 */
    ecan1MsgBuf[0][2] = 0x0006;

    // Write message 6 data bytes as follows:
    //
    // POTH POTL TEMPH TEMPL 0000 S3S2S1
    //
    ecan1MsgBuf[0][3] = (~SW1 & 1) | ((~SW2 & 1) << 1) | ((~SW3 & 1) << 2); // switch data, leading zeros
    ecan1MsgBuf[0][4] = TempValue;
    ecan1MsgBuf[0][5] = PotValue;

    Nop();
    Nop();
    Nop();
    /* Request message buffer 0 transmission */
    C1TR01CONbits.TXREQ0 = 0x1;
    /* The following shows an example of how the TXREQ bit can be polled to check if transmission
    is complete. */
    Nop();
    Nop();
    Nop();
    while (C1TR01CONbits.TXREQ0 == 1);
    // Message was placed successfully on the bus, return
}

void ADCInit(void) // will set 12bit, 4.96us/sample or 202KS/sec
{
    AD1CON1 = 0;            // POR: 10-bit @4ch mode, ADC disabled, manual sample
    AD1CON2 = 0;            // POR: AVdd/Avss for Vref, do not scan, IRQ every sample
    AD1CON3 = 0;            // POR: Use system clock, TAD = 1Tcyc, SAMC = 0TAD
    AD1CON4 = 0;            // POR: no DMA

    AD1CHS123 = 0;          // not used in 12bit mode, as only 1 S/H available
    AD1CON1bits.FORM = 0;   // integer data format (unsigned)
    AD1CON1bits.ASAM = 1;   // continuous automatic sampling enabled

    AD1CON3bits.ADCS = 8;   // 9 Tcy = 1TAD (so TAD = 9*25ns = 225ns = 4.44MHz)
    AD1CON3bits.SAMC = 8;   // set auto sample time as 8TAD = 1.8us


    AD1CON1bits.AD12B = 1;  // 12-bit conversion, 14TAD convert time

    AD1CON1bits.ADON = 1;   // enable converter

    //
    // Turn on port RG8, which supplies +5V to pot
    //
    _TRISG8 = 0;
    _LATG8 = 1;

    //
    // there is a delay time required from ADC ebable until application can begin converting
    // 150us is sufficient. Also allows pot voltage to stabilize, charges up anti-aliasing filter
    Delayus(150);

    AD1CON1bits.SAMP = 1; // begin continuous sampling/conversion

}

void ADCConvert(int channel)
//
// read the channel value
// averages 4 readings to reduce 'jitter'
{
    AverageValue = 0;
    for (i = 0; i < 4; i++)
    {
        AD1CHS0bits.CH0SA = channel;
        _AD1IF = 0;

        AD1CON1bits.SAMP = 0;
        Nop();
        Nop();
        Nop();
        Nop();
        Nop();
        Nop();
        while (!_AD1IF);
        AverageValue = AverageValue + ADC1BUF0;
    }

    AverageValue = AverageValue >> 2;

}
void InitMonitor(void)
{
    // digital output
    TRIS_MON = 0;

    //
    // map MONITOR_TX pin to port RB4, which is remappable RP36
    //
    RPOR1bits.RP36R = 0x03; // map UART2 TXD to pin RB4
    //
    // set up the UART for default baud, 1 start, 1 stop, no parity
    //
    U2MODEbits.STSEL = 0;       // 1-Stop bit
    U2MODEbits.PDSEL = 0;       // No Parity, 8-Data bits
    U2MODEbits.ABAUD = 0;       // Auto-Baud disabled
    U2MODEbits.BRGH = 0;        // Standard-Speed mode
    U2BRG = BAUD38400;          // Baud Rate setting for 38400 (default)
    U2STAbits.UTXISEL0 = 0;     // Interrupt after TX buffer done
    U2STAbits.UTXISEL1 = 1;
    IEC1bits.U2TXIE = 1;        // Enable UART TX interrupt
    U2MODEbits.UARTEN = 1;      // Enable UART (this bit must be set *BEFORE* UTXEN)

}

void Calc_Checksum(int data_byte)
{
    checksum = checksum + data_byte;        // add next
    if (checksum > 0xFF)
    {
        checksum = (checksum & 0xFF) + 1;   // truncate and add carry bit
    }
}

void delay_10ms(unsigned char num)
{
    f_tick = 0;                         //f_tick increments every 10ms
    while (f_tick < num);               // wait here until 'num' ticks occur
    f_tick = 0;
}

void Delayus(int delay)
{
    int i;
    for (i = 0; i < delay; i++)
    {
        __asm__ volatile ("repeat #39");
        __asm__ volatile ("nop");
    }
}

//*****************************************************************************
//
// Float to ASCII
//
// Converts a floating point number to ASCII. Note that buf must be
// large enough to hold result (in this case 4 digits)
//
// f is the floating point number.
// buf is the buffer in which the resulting string is placed.
//
// ftoa(1.23) returns "1.23"
//
//
//*****************************************************************************

void ftoa(float f, char *buf)
{
    int pos, ix, dp, num;
    pos = 0;
    ix = 0;
    dp = 0;
    num = 0;

    if (f < 0)
    {
        buf[pos++] = '-';
        f = -f;
    }
    dp = 0;
    while (f >= 10.0)
    {
        f = f / 10.0;
        dp++;
    }
    for (ix = 1; ix < (NUM_DIGITS + 1); ix++)
    {
        num = (int)f;
        f = f - num;
        buf[pos++] = '0' + num;
        if (dp == 0) buf[pos++] = '.';
        f = f * 10.0;
        dp--;
    }
}

void putsU2(char *s)
{
    while (*s)
    { // loop until *s =\0, end of string
        putU2(*s++);
    } // send the character and point to the next one
}

void putU2(int c)
{
    while (U2STAbits.UTXBF); // wait while Tx buffer full
    U2TXREG = c;
}

/******************************************************************************
*
*    Function:			rxECAN
*    Description:       moves the message from the DMA memory to RAM
*
*    Arguments:			*message: a pointer to the message structure in RAM
*						that will store the message.
******************************************************************************/
void rxECAN(mID *message)
{
	unsigned int ide=0;
	unsigned int rtr=0;
	unsigned long id=0;

	/*
	Standard Message Format:
	Word0 : 0bUUUx xxxx xxxx xxxx
			     |____________|||
 					SID10:0   SRR IDE(bit 0)
	Word1 : 0bUUUU xxxx xxxx xxxx
			   	   |____________|
						EID17:6
	Word2 : 0bxxxx xxx0 UUU0 xxxx
			  |_____||	     |__|
			  EID5:0 RTR   	  DLC
	word3-word6: data bytes
	word7: filter hit code bits

	Remote Transmission Request Bit for standard frames
	SRR->	"0"	 Normal Message
			"1"  Message will request remote transmission
	Substitute Remote Request Bit for extended frames
	SRR->	should always be set to "1" as per CAN specification

	Extended  Identifier Bit
	IDE-> 	"0"  Message will transmit standard identifier
	   		"1"  Message will transmit extended identifier

	Remote Transmission Request Bit for extended frames
	RTR-> 	"0"  Message transmitted is a normal message
			"1"  Message transmitted is a remote message
	Don't care for standard frames
	*/

	/* read word 0 to see the message type */
	ide=ecan1MsgBuf[message->buffer][0] & 0x0001;

	/* check to see what type of message it is */
	/* message is standard identifier */
	if(ide==0)
	{
		message->id=(ecan1MsgBuf[message->buffer][0] & 0x1FFC) >> 2;
		message->frame_type=CAN_FRAME_STD;
		rtr=ecan1MsgBuf[message->buffer][0] & 0x0002;
	}
	/* mesage is extended identifier */
	else
	{
		id=ecan1MsgBuf[message->buffer][0] & 0x1FFC;
		message->id=id << 16;
		id=ecan1MsgBuf[message->buffer][1] & 0x0FFF;
		message->id=message->id+(id << 6);
		id=(ecan1MsgBuf[message->buffer][2] & 0xFC00) >> 10;
		message->id=message->id+id;
		message->frame_type=CAN_FRAME_EXT;
		rtr=ecan1MsgBuf[message->buffer][2] & 0x0200;
	}
	/* check to see what type of message it is */
	/* RTR message */
	if(rtr==1)
	{
		message->message_type=CAN_MSG_RTR;
	}
	/* normal message */
	else
	{
		message->message_type=CAN_MSG_DATA;
		message->data[0]=(unsigned char)ecan1MsgBuf[message->buffer][3];
		message->data[1]=(unsigned char)((ecan1MsgBuf[message->buffer][3] & 0xFF00) >> 8);
		message->data[2]=(unsigned char)ecan1MsgBuf[message->buffer][4];
		message->data[3]=(unsigned char)((ecan1MsgBuf[message->buffer][4] & 0xFF00) >> 8);
		message->data[4]=(unsigned char)ecan1MsgBuf[message->buffer][5];
		message->data[5]=(unsigned char)((ecan1MsgBuf[message->buffer][5] & 0xFF00) >> 8);
		message->data[6]=(unsigned char)ecan1MsgBuf[message->buffer][6];
		message->data[7]=(unsigned char)((ecan1MsgBuf[message->buffer][6] & 0xFF00) >> 8);
		message->data_length=(unsigned char)(ecan1MsgBuf[message->buffer][2] & 0x000F);
	}
	clearRxFlags(message->buffer);
}

/******************************************************************************
*
*    Function:			clearRxFlags
*    Description:       clears the rxfull flag after the message is read
*
*    Arguments:			buffer number to clear
******************************************************************************/
void clearRxFlags(unsigned char buffer_number)
{
	if((C1RXFUL1bits.RXFUL1) && (buffer_number==1))
		/* clear flag */
		C1RXFUL1bits.RXFUL1=0;
	/* check to see if buffer 2 is full */
	else if((C1RXFUL1bits.RXFUL2) && (buffer_number==2))
		/* clear flag */
		C1RXFUL1bits.RXFUL2=0;
	/* check to see if buffer 3 is full */
	else if((C1RXFUL1bits.RXFUL3) && (buffer_number==3))
		/* clear flag */
		C1RXFUL1bits.RXFUL3=0;
	else;

}

/* code for Timer1 ISR, called every 250ms*/
void __attribute__((__interrupt__, no_auto_psv)) _T1Interrupt(void)
{
    s_tick++; // increment the 'slow tick'

    IFS0bits.T1IF = 0; //Clear Timer1 interrupt flag

}

/* code for Timer2 ISR, called every 10ms*/
void __attribute__((__interrupt__, no_auto_psv)) _T2Interrupt(void)
{
    f_tick++; // we increment the variable f_tick

    IFS0bits.T2IF = 0; //Clear Timer2 interrupt flag

}

void __attribute__((interrupt, no_auto_psv))_C1Interrupt(void)
{
    IFS2bits.C1IF = 0; // clear interrupt flag
    if (C1INTFbits.TBIF)
    {
        C1INTFbits.TBIF = 0;
    }

    if (C1INTFbits.RBIF)
    {

    /*check to see if buffer 1 is full */
    if(C1RXFUL1bits.RXFUL1)
    {
    /* set the buffer full flag and the buffer received flag */
    canRxMessage.buffer_status = CAN_BUF_FULL;
    canRxMessage.buffer = 1;
    can_rx = 1;
    }
    C1INTFbits.RBIF = 0;
    }
}

void __attribute__((interrupt, no_auto_psv)) _U1TXInterrupt(void)
{
    while (U1STAbits.TRMT == 0); // wait for transmitter empty
    IFS0bits.U1TXIF = 0; // Clear TX1 Interrupt flag
}

void __attribute__((interrupt, no_auto_psv)) _U2TXInterrupt(void)
{
    while (U2STAbits.TRMT == 0); // wait for transmitter empty
    IFS1bits.U2TXIF = 0; // Clear TX2 Interrupt flag
}

void __attribute__((interrupt, no_auto_psv)) _U2RXInterrupt(void)
{
    IFS1bits.U2RXIF = 0; // Clear RX2 Interrupt flag
}

void __attribute__((interrupt, no_auto_psv)) _DMA0Interrupt(void)
{
    IFS0bits.DMA0IF = 0; // Clear the DMA0 Interrupt Flag;
}

void __attribute__((interrupt, no_auto_psv)) _DMA1Interrupt(void)
{
    IFS0bits.DMA1IF = 0; // Clear the DMA1 Interrupt Flag;
}

void __attribute__((interrupt, no_auto_psv)) _DMA2Interrupt(void)
{
    IFS1bits.DMA2IF = 0; // Clear the DMA2 Interrupt Flag;
}

void __attribute__((interrupt, no_auto_psv)) _DMA3Interrupt(void)
{
    IFS2bits.DMA3IF = 0; // Clear the DMA3 Interrupt Flag;
}

void __attribute__((interrupt, auto_psv)) _DefaultInterrupt(void)
{
    LED1 = 1;
    LED2 = 1;
    LED3 = 1;

    while (1);
}

void __attribute__((interrupt, auto_psv)) _OscillatorFail(void)
{
    LED1 = 1;
    LED2 = 1;
    LED3 = 1;

    while (1);
}

void __attribute__((interrupt, no_auto_psv)) _MathError(void)
{
    LED1 = 1;
    LED2 = 1;
    LED3 = 1;

    while (1);
}

void __attribute__((interrupt, no_auto_psv)) _StackError(void)
{
    LED1 = 1;
    LED2 = 1;
    LED3 = 1;

    while (1);
}

void __attribute__((interrupt, no_auto_psv)) _AddressError(void)
{
    LED1 = 1;
    LED2 = 1;
    LED3 = 1;

    while (1);

}



