/******************************************************************************
* Progressive Nitrous Controller											  *
*																			  *
*																			  *
* File Name:																  *
*																			  *				 
* Name:																		  *
* Date:																		  *
*																			  * 
* Description: 																  *
*																			  *
*																			  *
*																			  *
*																			  *
*  			 ATmega328 pin out:											      *
*																			  *
*							 _______________________						  *
*						1___| PC6/RESET*		PC5 |___28					  *
*							|						|						  *
*						2___| PD0/RXD			PC4	|___27					  *
*							|						|						  *
*		Display_________3___| PD1/TXD			PC3 |___26					  *
*							|						|						  *
*						4___| PD2/INT0			PC2 |___25					  *
*						    |						|						  *
*						5___| PD3/INT1		    PC1 |___24					  *
*							|						|						  *
*  						6___| PD4				PC0 |___23					  *
*							|						|						  *
*						7___| VCC				GND |___22					  *
*							|						|						  *
*						8___| GND			   AREF |___21					  *
*						 	|						|						  *
*						9___| PB6/XTAL1	       AVCC |___20					  *
*							|						|						  *
*					   10___| PB7/XTAL2		PB5/SCK	|___19					  *
*							|						|						  *
*					   11___| PD5		   PB4/MISO	|___18					  *
*							|						|						  *
*		INCREMENT______12___| PD6		   PB3/MOSI	|___17					  *
*							|						|						  *
*		DECREMENT______13___| PD7				PB2	|___16 ____				  *
*							|						|						  *
*					   14___| PB0				PB1 |___15					  *
*							|_______________________|						  *
*							 												  *
 *****************************************************************************/ 

#define	F_CPU		1000000UL		// Set clock rate to 1MHz

#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdint.h>

#include "serial_comm_1MHz.c"

void init_UART(void);
int engagementRPM = 5000; //max is redline
int RPM;
int progressRate = 50; //max is redline
double dutyCycle;
double TPS; //0 to 255
int redLine = 10000; //max RPM value or Y value
double engineLoad;
int underLoadProgress = 50;


#define	LED_PIN			PB0

#define ADC_THRESHOLD	256

#define ADC_PIN			0


int isback = 0;

int waitcount = 0;
#define MAXWAITCOUNT  0
#define WAITAMOUNT 50


#define MINWAIT 15
#define MAXWAIT 100
int isSetup = 1;
int curPage = 1;
//display engagement RPM 0 to 13000 incrementing by 10
//display progressRatePerentage 500 to 0
//x = duty cycle percentage
//if the button is held scroll at increased rate

int hasHeldUpDelay = MAXWAIT; //in milliseconds
int hasHeldDownDelay = MAXWAIT; //in milliseconds
	//buttonUp = 0x08 PIND
	//buttonDown = 0x10 PIND
	//buttonRight  = 0x40 PIND
	//buttonLeft = 0x01 PINB
	//buttonBack = 0x04 PINB
	//buttonEnter = 0x02 PINB
int buttonUp(){
	if ((PIND & 0x08)){
		return 0;
	}else{
		return 1;	
	}
}
int buttonDown(){
	if ((PIND & 0x10)){
		return 0;
	}else{
		return 1;	
	}
}
int buttonRight(){
	if ((PIND & 0x40)){
		return 0;
	}else{
		return 1;	
	}
}
int buttonLeft(){
	if ((PINB & 0x01)){
		return 0;
	}else{
		return 1;	
	}
}
int buttonEnter(){
	if ((PINB & 0x02)){
		return 0;
	}else{
		return 1;	
	}
}
int buttonBack(){
	if ((PINB & 0x04)){
		return 0;
	}else{
		return 1;	
	}
}


uint16_t adc_read(uint8_t adcx) {
	 	/* adcx is the analog pin we want to use.  ADMUX's first few bits are
	 	 * the binary representations of the numbers of the pins so we can
	 	 * just 'OR' the pin's number with ADMUX to select that pin.
	 	 * We first zero the four bits by setting ADMUX equal to its higher
	 	 * four bits. */
 	ADMUX	&=	0xf0;
 	ADMUX	|=	adcx;
	
	
	 	/* This starts the conversion. */
 	ADCSRA |= _BV(ADSC);

	
 	/* This is an idle loop that just wait around until the conversion
 	 * is finished.  It constantly checks ADCSRA's ADSC bit, which we just
	   	 * set above, to see if it is still set.  This bit is automatically
	 	 * reset (zeroed) when the conversion is ready so if we do this in
	 	 * a loop the loop will just go until the conversion is ready. */
	 	while ( (ADCSRA & _BV(ADSC)) );
	
	
	 	/* Finally, we return the converted value to the calling function. */
	 	return ADC;
}


void wait(int mills){
	int i;
	for(i = 0; i < mills;i++){
		_delay_ms(1);
	}
}
int handleRedLineInput(){
	while(1){
		if(buttonEnter() == 1){
			while(buttonEnter() == 1){
				//wait until they release the button
			}
			engagementRPM = 1000;
			return redLine;
		}
		if(buttonUp() == 1){
			redLine+=50;
			if(redLine>=20000){
				redLine=20000;
			}
			if(waitcount >= MAXWAITCOUNT){
				clearLCD();
				setCursor(line1);
				printf("Red Line / Shift RPM");
				setCursor(line2);
				printf("     Set: %d",redLine);
				setCursor(line4);
				printf("Press Enter to Cont.");
				waitcount = 0;
			}else{
				waitcount++;
			}
			wait(hasHeldUpDelay); //IMPORTANT: not sure if the sleep function should be used here, essentially need something that will pause for a given amount of time in milliseconds
			hasHeldUpDelay = (int)((double)hasHeldUpDelay * 7/10); //over time reduce the delay exponentially
			if(hasHeldUpDelay <= MINWAIT){
				hasHeldUpDelay = MINWAIT; //shortest possible delay is 50
			}
			}else{ //if both the up and down are pressed at the same time, the up will be registered over the down
			hasHeldUpDelay = MAXWAIT;
			if(buttonDown() == 1){
				redLine-=50;
				if(redLine<=1000){
					redLine=1000;
				}
				wait(hasHeldDownDelay); //IMPORTANT: not sure if the sleep function should be used here, essentially need something that will pause for a given amount of time in milliseconds
				hasHeldDownDelay = (int)((double)hasHeldDownDelay * 7/10); //over time reduce the delay exponentially
				if(hasHeldDownDelay <= MINWAIT){
					hasHeldDownDelay = MINWAIT; //shortest possible delay is 50
				}
				}else{
				hasHeldDownDelay = MAXWAIT;
			}
		}
		if(waitcount >= MAXWAITCOUNT){
			clearLCD();
			setCursor(line1);
			printf("Red Line / Shift RPM");
			setCursor(line2);
			printf("     Set: %d",redLine);
			setCursor(line4);
			printf("Press Enter to Cont.");
			waitcount = 0;
			}else{
			waitcount++;
		}
		wait(WAITAMOUNT);
	}
}


int handleEngagementRPM(){
	
	while(1){
		if(buttonEnter() == 1){
			while(buttonEnter() == 1){
				//wait until they release the button
			}
			return engagementRPM;
		}
		if(buttonUp() == 1){
			engagementRPM+=50;
			if(engagementRPM>=redLine){
				engagementRPM=redLine;
			}
			if(waitcount >= MAXWAITCOUNT){
				clearLCD();
				setCursor(line1);
				printf("   Engagement RPM");
				setCursor(line2);
				printf("     Set: %d",engagementRPM);
				setCursor(line4);
				printf("Press Enter to Cont.");
				waitcount = 0;
			}else{
				waitcount++;
			}
			wait(hasHeldUpDelay); //IMPORTANT: not sure if the sleep function should be used here, essentially need something that will pause for a given amount of time in milliseconds
			hasHeldUpDelay = (int)((double)hasHeldUpDelay * 7/10); //over time reduce the delay exponentially
			if(hasHeldUpDelay <= MINWAIT){
				hasHeldUpDelay = MINWAIT; //shortest possible delay is 50
			}
			}else{ //if both the up and down are pressed at the same time, the up will be registered over the down
			hasHeldUpDelay = MAXWAIT;
			if(buttonDown() == 1){
				engagementRPM-=50;
				if(engagementRPM<=0){
					engagementRPM=0;
				}
				wait(hasHeldDownDelay); //IMPORTANT: not sure if the sleep function should be used here, essentially need something that will pause for a given amount of time in milliseconds
				hasHeldDownDelay = (int)((double)hasHeldDownDelay * 7/10); //over time reduce the delay exponentially
				if(hasHeldDownDelay <= MINWAIT){
					hasHeldDownDelay = MINWAIT; //shortest possible delay is 50
				}
				}else{
				hasHeldDownDelay = MAXWAIT;
			}
		}
		if(buttonBack() == 1){ //if they press the back button
			while(buttonBack() == 1){
				//wait until they release the button
			}
			curPage = 1;
			return -1;
		}
		if(waitcount >= MAXWAITCOUNT){
			clearLCD();
			setCursor(line1);
			printf("   Engagement RPM");
			setCursor(line2);
			printf("     Set: %d",engagementRPM);
			setCursor(line4);
			printf("Press Enter to Cont.");
			waitcount = 0;
		}else{
			waitcount++;
		}
		wait(WAITAMOUNT);
	}
}

int handleProgressRate(){
	while(1){
		if(buttonEnter() == 1){
			while(buttonEnter() == 1){
				//wait until they release the button
			}
			return progressRate;
		}
		if(buttonUp() == 1){
			progressRate+=1;
			if(progressRate>=100){
				progressRate=100;
			}
			if(waitcount >= MAXWAITCOUNT){
				clearLCD();
				setCursor(line1);
				printf("    Progress Rate");
				setCursor(line2);
				printf("      Set: %d%%",progressRate);
				setCursor(line4);
				printf("Press Enter to Cont.");
				waitcount = 0;
			}else{
				waitcount++;
			}
			wait(hasHeldUpDelay); //IMPORTANT: not sure if the sleep function should be used here, essentially need something that will pause for a given amount of time in milliseconds
			hasHeldUpDelay = (int)((double)hasHeldUpDelay * 7/10); //over time reduce the delay exponentially
			if(hasHeldUpDelay <= MINWAIT){
				hasHeldUpDelay = MINWAIT; //shortest possible delay is 50
			}
		}else{ //if both the up and down are pressed at the same time, the up will be registered over the down
			hasHeldUpDelay = MAXWAIT;
			if(buttonDown() == 1){
				progressRate = progressRate - 1;
				if(progressRate<=0){
					progressRate=0;
				}
				wait(hasHeldDownDelay); //IMPORTANT: not sure if the sleep function should be used here, essentially need something that will pause for a given amount of time in milliseconds
				hasHeldDownDelay = (int)((double)hasHeldDownDelay * 7/10); //over time reduce the delay exponentially
				if(hasHeldDownDelay <= MINWAIT){
					hasHeldDownDelay = MINWAIT; //shortest possible delay is 50
				}
				}else{
				hasHeldDownDelay = MAXWAIT;
			}
		}
		if(buttonBack() == 1){ //if they press the back button
			while(buttonBack() == 1){
				//wait until they release the button
			}
			return -1;
		}
		if(waitcount >= MAXWAITCOUNT){
			clearLCD();
			setCursor(line1);
			printf("    Progress Rate");
			setCursor(line2);
			printf("      Set: %d%%",progressRate);
			setCursor(line4);
			printf("Press Enter to Cont.");
			waitcount = 0;
		}else{
			waitcount++;
		}
		wait(WAITAMOUNT);
	}
}


int handleUnderLoadProgress(){
	while(1){
		if(buttonEnter() == 1){
			while(buttonEnter() == 1){
				//wait until they release the button
			}
			return underLoadProgress;
		}
		if(buttonUp() == 1){
			underLoadProgress+=1;
			if(underLoadProgress>=100){
				underLoadProgress=100;
			}
			if(waitcount >= MAXWAITCOUNT){
				clearLCD();
				setCursor(line1);
				printf(" Under Load Progress");
				setCursor(line2);
				printf("      Set: %d%%",underLoadProgress);
				setCursor(line4);
				printf("Press Enter to Cont.");
				waitcount = 0;
			}else{
				waitcount++;
			}
			wait(hasHeldUpDelay); //IMPORTANT: not sure if the sleep function should be used here, essentially need something that will pause for a given amount of time in milliseconds
			hasHeldUpDelay = (int)((double)hasHeldUpDelay * 7/10); //over time reduce the delay exponentially
			if(hasHeldUpDelay <= MINWAIT){
				hasHeldUpDelay = MINWAIT; //shortest possible delay is 50
			}
		}else{ //if both the up and down are pressed at the same time, the up will be registered over the down
			hasHeldUpDelay = MAXWAIT;
			if(buttonDown() == 1){
				underLoadProgress = underLoadProgress - 1;
				if(underLoadProgress<=0){
					underLoadProgress=0;
				}
				wait(hasHeldDownDelay); //IMPORTANT: not sure if the sleep function should be used here, essentially need something that will pause for a given amount of time in milliseconds
				hasHeldDownDelay = (int)((double)hasHeldDownDelay * 7/10); //over time reduce the delay exponentially
				if(hasHeldDownDelay <= MINWAIT){
					hasHeldDownDelay = MINWAIT; //shortest possible delay is 50
				}
				}else{
				hasHeldDownDelay = MAXWAIT;
			}
		}
		if(buttonBack() == 1){ //if they press the back button
			while(buttonBack() == 1){
				//wait until they release the button
			}
			return -1;
		}
		if(waitcount >= MAXWAITCOUNT){
			clearLCD();
			setCursor(line1);
			printf(" Under Load Progress");
			setCursor(line2);
			printf("      Set: %d%%",underLoadProgress);
			setCursor(line4);
			printf("Press Enter to Cont.");
			waitcount = 0;
		}else{
			waitcount++;
		}
		wait(WAITAMOUNT);
	}
}



double calculateActualSlope(){
	double returnval = (-5 * progressRate) + 500;
	return returnval;
}


void calculateEngineLoad(){
	double upper = redLine * 5;
	engineLoad = RPM*TPS;
	double final = (upper - engineLoad)/upper; // upper= 100,000

}

double calculateUnderLoad(){
	double finalVal = (calculateEngineLoad()-engagementRPM)/calculateActualSlope();
	return finalVal;

}

double calculateDutyCycle(){
	double finalVal = (RPM-engagementRPM)/calculateActualSlope();
	return finalVal;

}
void init_A2D()
{
	ADMUX = 0x80;
	
	ADMUX |= (1 << MUX3);
	
	ADMUX |= (1 << MUX0);
	
	ADCSRB = 0x10;
	
	ADCSRA |= (1 << ADATE);
	
	ADCSRA |= (1 << ADEN);
	
	ADCSRA |= (1 << ADPS1);
	
	ADCSRA |= (1 << ADPS0);
	
	//DIDR1  |= (1 << ADC9D);
}


int main(void)
{
	/*********** Initializations *******************/
	
	init_UART();
	_delay_ms(250);
	clearLCD();
		
	DDRC  =		0x06;
	PORTC = 	0x06;
	DDRD  =     0xDC; 
	PORTD =		0xDC; 
	PORTB =		0x07;
	DDRB =		0x07;
	/****************** Main Loop **************************/
    /*ADCSRA |= _BV(ADEN);

    DDRB  |= _BV(LED_PIN);
		uint16_t val = 44;
    	init_A2D(); //Initialize A/D Registers
    	
    	ADCSRA |= (1 << ADSC);
    	
    	while(1)
    	{
	    	val = ADCL;
	    	
	    	val += ADCH * 256;
	    	
	    	numDisplay(val);
    	}
*/

	int i;
	int j;
	for(i = 0; i < 25; i++){
		clearLCD();
		setCursor(line2);
		printf(" Wisher Performance");
		setCursor(line3);
		printf("   Total Kontrol");
		wait(200);
	}
    while(1){
	   if(isSetup==1){ //if we are currently setting up
		    if(curPage == 1){
			    redLine = handleRedLineInput();
			    curPage = 2; //continue on to the next page in the next iteration of the loop
			}else if(curPage == 2){
				j = handleEngagementRPM();
				if(j == -1){
					curPage = 1;
				}else{
					curPage = 3;
					engagementRPM = j;
				}
			}else if(curPage == 3){
				j = handleProgressRate();
				if(j == -1){
					curPage = 2;
				}else{
					curPage = 4;
					progressRate = j;
				}
		    }else if(curPage == 4){
				j = handleUnderLoadProgress();
				if(j == -1){
					curPage = 3;
				}else{
					isSetup = 0;
					curPage = 1;//done setting up values, start executing
					underLoadProgress = j;
				}
			}
		}else{ //if we are currently executing
			clearLCD();
			setCursor(line2);
			printf("     RPM:%dTPS:%d",RPM,(int)TPS);
			calculateEngineLoad();
			waitcount= 0;
			if(buttonBack() == 1){ //if they press the back button
				while(buttonBack() == 1){
					//wait until they release the button
				}
				isSetup = 1;
				curPage=3;
			}
		    if (RPM >= engagementRPM ){
			    if(TPS > 4.8){
				    double x = calculateDutyCycle();
					double x2 = calculateUnderLoad();
				    if(x2 < x){
				    	setCursor(line3);
				   		printf("    Duty Cycle: %d",(int)x);
				    }else{
				    	setCursor(line3);
				   		printf("    Under Load: %d",(int)x2);
				    }
					
					wait(WAITAMOUNT);
				    //do weird shit with x
			    }
		    }
			wait(WAITAMOUNT);
	    }
    }


}	
