#include <stdint.h>
#include "fdc.h"

int main ()
{
	char *ptr = "Hello world!";
	char *np = 0;
	int i = 5;
	unsigned int bs = sizeof(int)*8;
	int mi;
	char buf[80];

	if ((fdc_init()) != FDC_SUCCESS)
		while (1);
	do {	
		// replace with normal printf call
		fdc_printf ("Hello World\n");
		fdc_printf ("H.e.l.l.o.W.o.r.l.d\n");

/*
 * this should display (on 32bit int machine) :
 *
 * Hello world!
 * printf test
 * (null) is null pointer
 * 5 = 5
 * -2147483647 = - max int
 * char a = 'a'
 * hex ff = ff
 * hex 00 = 00
 * signed -3 = unsigned 4294967293 = hex fffffffd
 * 0 message(s)
 * 0 message(s) with %
 * justif: "left      "
 * justif: "     right"
 *  3: 0003 zero padded
 *  3: 3    left justif.
 *  3:    3 right justif.
 * -3: -003 zero padded
 * -3: -3   left justif.
 * -3:   -3 right justif.
 */
		mi = (1 << (bs-1)) + 1;
		fdc_printf("%s\n", ptr);
		fdc_printf("fdc_printf test\n");
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
		fdc_sprintf(buf, "justif: \"%-10s\"\n", "left"); fdc_printf("%s", buf);
		fdc_sprintf(buf, "justif: \"%10s\"\n", "right"); fdc_printf("%s", buf);
		fdc_sprintf(buf, " 3: %04d zero padded\n", 3); fdc_printf("%s", buf);
		fdc_sprintf(buf, " 3: %-4d left justif.\n", 3); fdc_printf("%s", buf);
		fdc_sprintf(buf, " 3: %4d right justify\n", 3); fdc_printf("%s", buf);
		fdc_sprintf(buf, "-3: %04d zero padded\n", -3); fdc_printf("%s", buf);
		fdc_sprintf(buf, "-3: %-4d left justif.\n", -3); fdc_printf("%s", buf);
		fdc_sprintf(buf, "-3: %4d right justify\n", -3); fdc_printf("%s", buf);

		fdc_printf("Done\n\0");

	} while (1);
}
