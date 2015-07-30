#include "fdc.h"
#include "fdc_putchar.h"

static unsigned long fdcBaseAddr;
static int cdmm_was_mapped = 0;
static FDC_U32 gCDMMBase;
static int fdc_char_count = 0;

void fdc_putchar(int c) __attribute__ ((noinline));
void udelay (unsigned usec) __attribute__ ((noinline));
void mdelay (unsigned msec) __attribute__ ((noinline));

#define C0_COUNT            9   /* Processor cycle count */

/*
* Delay for a given number of microseconds.
* The processor has a 32-bit hardware Count register,
* which increments at half CPU rate.
* We use it to get a precise delay.
*/


/********************************************************************************************
 *                                                                                          *
 * fdc_hw_find_base_addr - is used to local the base address of the FDC memory mapped       *
 * registers. The FDC memory-mapped registers are located in the common device memory       *
 * map (CDMM) region. FDC has a device ID of 0xFD. If FDC not found return                  *
 * FDC_ERR_NO_FDC_HW_FOUND otherwise return FDC_SUCCESS.                                    *
 *                                                                                          *
 ********************************************************************************************/
int fdc_hw_find_base_addr(unsigned long *fdcBaseAddr) {
	int result;
	int found = 0;
	FDC_U32 CDMMBase;
	FDC_U32 CDMMBaseAddr;
	FDC_U32 numdrbs;
	CDMM_DEVICE_RESOURCE_BLOCK *drb_begin;
	CDMM_DEVICE_RESOURCE_BLOCK *drb_end;
	CDMM_DEVICE_RESOURCE_BLOCK *drb_cur;			

	CDMMBase = gCDMMBase;
	CDMMBaseAddr = (CDMMBase >> 11) << 15;
	numdrbs = (CDMMBase & 0x1FF);
	drb_begin = (CDMM_DEVICE_RESOURCE_BLOCK*)CDMMBaseAddr;
	drb_end = drb_begin + numdrbs;

	drb_cur = ((CDMMBase & (1 << 9)) == 0) ? drb_begin : drb_begin+1;
	while (drb_cur < drb_end) {
		FDC_U32 acsr = drb_cur->primary.acsr;
		FDC_U32 extradrbs;

		if (CDMM_DEV_TYPE(acsr) == 0xFD) {
			found = 1;
			break;
		} 
		extradrbs = CDMM_DEV_SIZE(acsr);
		drb_cur += extradrbs;
	}
	if (found) {
		*fdcBaseAddr = (unsigned long) drb_cur;
		result = FDC_SUCCESS;
	} else {
		result = FDC_ERR_NO_FDC_HW_FOUND;
	}
	
	return result;
}

/********************************************************************************************
 *                                                                                          *
 * fdc_init is called to emable the Common Device Memory Map and save global pointer to     *
 * CDMM.                                                                                    *
 *                                                                                          *
 *                                                                                          *
 ********************************************************************************************/
int fdc_init(void) {
	int result = FDC_SUCCESS;
	FDC_U32 CDMMOverlayPhysicalAddr = 0x1fc10000;

	if (!IS_PHYSICAL_ADDRESS_ADDRESSABLE_VIA_KSEG1(CDMMOverlayPhysicalAddr))
		result = FDC_ERR_CDMM_BASE_ADDR_NOT_ACCESSIBLE_VIA_KSEG1;

	if (result == FDC_SUCCESS) {
		if ((CDMMOverlayPhysicalAddr & ((1 << 15)-1)) != 0) {
			result = FDC_ERR_CDMM_BASE_ADDR_BAD_ALIGNMENT;
		}
	}

	if (result == FDC_SUCCESS) {

		FDC_U32 kseg1Addr = MAKE_KSEG1_ADDR(CDMMOverlayPhysicalAddr);
		FDC_U32 config3 = ReadConfig3();

		if ((config3 & 0x8) != 0) {

			FDC_U32 CDMMBase = ReadCDMMBase();

			/* Is Memory-Mapped I/O enabled */
			if (((CDMMBase & 0x400) >> 10) != 1)
			{
				// set address
				cdmm_was_mapped = (CDMMBase & (1 << 10)) != 0;

				// enable
				CDMMBase |= (1 << 10);				
				WriteCDMMBase(CDMMBase);
				CDMMBase |= ((kseg1Addr >> 15) << 11);
			}

			gCDMMBase = CDMMBase;

		} else {
			result = FDC_ERR_PROCESSOR_HAS_NO_CDMM;
		}

	}

	fdc_hw_find_base_addr(&fdcBaseAddr);
	fdc_char_count = 0;
	return result;
}

/********************************************************************************************
 *                                                                                          *
 * fdc_putchar writes a character to 4 byte null terminated char buffer. Once buffer is     *
 * full the 4 character are tranmitted using the FDC channel zero. If the FDC FIFO is full  *
 * then poll for FIFO not full or timeout waiting. The timeout is used just in case OpenOCD *
 * does not have semi-hosting enabled. Future enhancement add control channel to pass info  *
 * like semi-hosting enable/disabled.                                                       *
 *                                                                                          *
 * FDC Channel Zero only with Polling. Interrupts not support at this time                  *
 *                                                                                          *
 ********************************************************************************************/
void fdc_putchar(int c)
{
	FDC_REGS *fdc = (FDC_REGS *)fdcBaseAddr;
	volatile int timeout = false;
	long long i;

	if (fdc_char_count == 0) {
		fdc_buff.x.l = 0;
	}

	if ((fdc_char_count > 2) || (c == '\0')) {
		/* Insert Char. in buffer */
		fdc_buff.x.c_buff[fdc_char_count] = c;

		// If Tx FIFO full wait for not full or timeout
		i = 0;
		while (((fdc->fdstat) & 0x1) == 1) {
			if (i >= FDC_PUTCHAR_TIMEOUT) {
				timeout = true;
				break;
			}
			timeout = false;
			i++;
			mdelay (4); /* delay for 4 msec */
		}

		/* Check for timeout - host may have semihosting enabled */
		if (timeout != true) {
			fdc->fdtx = fdc_buff.x.l;
		}

		fdc->fdtx = fdc_buff.x.l;

		fdc_buff.x.l = 0;
		fdc_char_count = 0;
	}
	else
		{
			fdc_buff.x.c_buff[fdc_char_count++]=c;
		}
}

#pragma GCC optimize ("O0")
void udelay (unsigned usec)
{
    volatile unsigned now = ReadCount();
    volatile unsigned final = now + usec * MHZ / 2;

    for (;;) {
        volatile unsigned now = ReadCount();

        /* This comparison is valid only when using a signed type. */
        if ((volatile int) (now - final) >= 0)
            break;
    }
}


void mdelay (unsigned msec)
{
    while (msec-- > 0) {
        udelay(1000);
    }
}
