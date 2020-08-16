#include <stdint.h>
#include "fdc.h"

void delay();

//-------------------------------------------------------
// main()
//
// Test code for the fdc_printf routines.  This is designed for the Digilent Nexys4 DDR board,
// running the microAptiv MIPS32 core.  The program uses the 5 pushbuttons and 16 LEDs.
//
//-------------------------------------------------------

int main()
{
        volatile int *IO_LEDR = (int*)0xbf800000;
        volatile int *IO_PUSHBUTTONS = (int*)0xbf80000c;

        volatile unsigned int pushbutton;
        volatile unsigned short count = 0;

        fdc_init();  // initialize the CDMM hardware which memory maps the FDC FIFO and status regs

        while (1) {
           pushbutton = *IO_PUSHBUTTONS;

           switch (pushbutton) {
                   case 0x1: { // right
                        if (count==0) count = 0xf000;
                        else count = count >> 1;
                        fdc_printf("1");
                        fdc_printf("1\n");
                        fdc_printf("12\n");
                        fdc_printf("123\n");
                        break;
                        }
                   case 0x2: { // middle
                        if (count==0) count = ~count;
                        else count = 0;
                        
                        char *ptr = "Hello world!";
                        char *np = 0;
                        int i = 5;
                        unsigned int bs = sizeof(int)*8;
                        int mi;
                        char buf[80];

                        mi = (1 << (bs-1)) + 1;
                        fdc_printf("%s\n", ptr);
                        fdc_printf("printf test\n");
                        fdc_printf("%s is null pointer\n", np);
                        fdc_printf("%d = 5\n", i);
                        fdc_printf("%d = - max int\n", mi);
                        fdc_printf("char %c = 'a'\n", 'a');
                        fdc_printf("hex %x = ff\n", 0xff);
                        fdc_printf("hex %02x = 00\n", 0);
                        fdc_printf("signed %d = unsigned %u = hex %x\n", -3, -3, -3);
                        fdc_printf("%d %s(s)%", 0, "message");
                        fdc_printf("\n");
                        fdc_printf("%d %s(s) with %%\n", 0, "message");
                        fdc_printf("justif: \"%-10s\"\n", "left");
                        fdc_sprintf(buf, "justif: \"%-10s\"\n", "left");
                        fdc_printf("%s", buf);
                        fdc_sprintf(buf, "justif: \"%10s\"\n", "right");
                        fdc_printf("%s", buf);
                        fdc_sprintf(buf, " 3: %04d zero padded\n", 3); fdc_printf("%s", buf);
                        fdc_sprintf(buf, " 3: %-4d left justif.\n", 3); fdc_printf("%s", buf);
                        fdc_sprintf(buf, " 3: %4d right justif.\n", 3); fdc_printf("%s", buf);
                        fdc_sprintf(buf, "-3: %04d zero padded\n", -3); fdc_printf("%s", buf);
                        fdc_sprintf(buf, "-3: %-4d left justif.\n", -3); fdc_printf("%s", buf);
                        fdc_sprintf(buf, "-3: %4d right justif.\n", -3); fdc_printf("%s", buf);
                        
                        break;
                             }
                   case 0x4: { // left
                        count--;
                        fdc_printf("ABCDEFGHIJKLMNOPQRSTUVWXYZ abcdefghijklmnopqrstuvwxyz 0123456789\n");
                        break;
                        }
                   case 0x8: { // bottom
                        // test speed of output - 1000 outputs of fixed length
                        for (count = 64000; count < 65000; count++) {
                            fdc_printf("%d\n", count);              
                           }
                        break;
                        }
                   case 0x10: { //top
                        fdc_printf("count = %d\n", count++);
                        break;
                        }
                   default: if (count==0) count = 0xf;
                             else count = count << 1;
                } //end switch
           *IO_LEDR = count;            // write to green LEDs

           delay();

        } // end while(1)
        return 0;
} // end of main()

void delay() {
   volatile unsigned int j;

   for (j = 0; j < (120000); j++) ;  // set for caches enabled
} // end of delay()
