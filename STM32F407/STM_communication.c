#include "stm32f4xx.h" //device header
#include "LCD_init.h"  // file for LCD display
#include "weights.h"    // W1, b1, W2, b2

/*The comunication between the USB camera and STM is through RAM. Memory is stored
at a specific location: "MNISTBOX". The bytes used to send information are at 
values offset within this mailbox. The first 8 bytes store "MNIS" and "TBOX"...then
the other important information is at offset values. 

Information about button presses (or space presses in the python code) is at an 
offset of 8 bytes. Commands is at 12 bytes of offset, result (the predicted value) is at 16 bytes
of offset, and the image is at 20 bytes of offset. 
*/

typedef struct {
		//PC searches RAM for magic1 and magic2 (spells "MNIS" "TBOX")
    uint32_t magic1;
    uint32_t magic2;
    volatile uint32_t button;    // offset 8 bytes
    volatile uint32_t cmd;       // offset 12 bytes
    volatile uint32_t result;    // offset 16 bytes
    volatile uint8_t  img[784];  // offset 20 bytes
} mailbox_t; //struct named mailbox_t

volatile mailbox_t mbox = { 0x4D4E4953u, 0x54424F58u, 0, 0, 0, {0} };

float x[N_IN]; // input image (input layer ->784 neurons)
float A1[N_HID]; //hidden layer outputs (100 neurons)
float Z2[N_OUT]; //output layer (10 neurons...one per digit)


// Run the network on x[] and return the predicted digit 0-9
static int nn_predict(void)
{
    // Layer 1: A1 = ReLU(W1*x + b1) 
    for (int i = 0; i < N_HID; i++) { //run for total number of hidden neurons
			
			//each neuron needs to add up to a total
        float sum = b1[i]; //start the sum at the bias value
        for (int j = 0; j < N_IN; j++) { //walk through every 784 pixel of the input layer
            sum = sum + W1[i * N_IN + j] * x[j]; //apply the equation for Z1
        }
        if (sum < 0) { //ReLU function
            sum = 0;
        }
        A1[i] = sum; //store result into A1
    }

    // Layer 2: Z2 = W2*A1 + b2 
    for (int i = 0; i < N_OUT; i++) { //10 output neurons
        float sum = b2[i]; //start sum at bias value
        for (int j = 0; j < N_HID; j++) { //walk through every hidden neuron
            sum = sum + W2[i * N_HID + j] * A1[j]; //applying equation for Z2
        }
        Z2[i] = sum; //store the result into Z2
    }

    // Biggest score = predicted digit (softmax is skipped because the biggest score will still be the biggest probability
		//Softmax is only truly necessary in training the neural network
    int best = 0;
    for (int i = 1; i < N_OUT; i++) { //work through all output values
        if (Z2[i] > Z2[best]) {
            best = i;
        }
    }
    return best; //return the prediction
}

static void process_image(void)
{
    for (int i = 0; i < 784; i++) { //loop over each pixel
        x[i] = mbox.img[i] / 255.0f; // normalize pixels from 0-1 
    }
    mbox.result = nn_predict(); //enter prediction into mbox (struct) in the result slot
}

int main(void)
{
    //Set button as input==================================
		//clock
		RCC -> AHB1ENR |= (1<<0); //enable port A clock (using masking)
	
		//MODER: 00 for input
		//button: pin 0
		GPIOA -> MODER &= ~(1<<1); //bit 1 = 0
		GPIOA -> MODER &= ~(1<<0); //bit 0 = 0
		//Pull up/down: set to 00
		GPIOA -> PUPDR &=~(1<<1); //Sets bit PUPDR OFF
		GPIOA -> PUPDR &=~(1<<0); //Sets bit PUPDR OFF
		//OTYPER: set bit to 0
		GPIOA -> OTYPER &=~(1<<0); //Sets bit 0 in OTYPER OFF
		//=======================================================

		//Button interrupt (EXTI0)===============================
		RCC -> APB2ENR |= RCC_APB2ENR_SYSCFGEN; //enable SYSCFG clock
		SYSCFG -> EXTICR[0] &= ~(0xF<<0); //EXTI0 connected to Port A (0000)
		EXTI -> IMR |= (1<<0); //unmask line 0 (allow the interrupt)
		EXTI -> RTSR |= (1<<0); //trigger on rising edge (button press)
		EXTI -> FTSR &= ~(1<<0); //do not trigger on falling edge (release)
		///////////////////////////////////////////////
		// Let CMSIS take care of NVIC interrupt regs
		__enable_irq(); //interrupt
		NVIC_SetPriority(EXTI0_IRQn,0); //set priority to 0
		NVIC_ClearPendingIRQ(EXTI0_IRQn); //clear pending interrupts
		NVIC_EnableIRQ(EXTI0_IRQn); //enable the NVIC
		// DONE WITH NVIC
		///////////////////////////////////////////////

    LCD_port_init(); //initialize LCD port 
    LCD_init();//initialize LCD
    LCD_String_Write("Ready"); //display "Ready"

    while (1) {
        //button press is handled by EXTI0_IRQHandler (below main)

        if (mbox.cmd == 1) {                  /* PC says: image is ready */
            process_image();                  /* run the network */
            mbox.cmd = 2;                     /* tell PC: done */

            ClearLCD();                       /* clear + cursor to line 1 */
            LCD_String_Write("Digit: ");
            LCD_write('0' + mbox.result);     /* 7 -> '7' */
        }
    }
}

//=============================================================================
// EXTI0 Interrupt Handler (blue user button on PA0)
// Runs automatically when the button is pressed (rising edge).
// The pending flag is cleared by writing a 1 to it.
//=============================================================================
void EXTI0_IRQHandler(void)
{
	if (EXTI -> PR & (1<<0)){ //check if line 0 flag is set (button pressed)
		mbox.button = 1; //tell the PC the button was pressed
		EXTI -> PR = (1<<0); //clear flag (write 1 to clear)
	}
}