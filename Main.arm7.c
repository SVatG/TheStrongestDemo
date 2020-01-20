#include <nds.h>
#include <nds/fifocommon.h>
#include <maxmod7.h>

int main(int argc, char ** argv)
{
	//enable  sound 
	powerON(POWER_SOUND); 
	writePowerManagement(PM_CONTROL_REG,(readPowerManagement(PM_CONTROL_REG)&~PM_SOUND_MUTE)|PM_SOUND_AMP );

	irqInit();
	irqEnable(IRQ_VBLANK);
	fifoInit();
 
	mmInstall(11,MM_TIMER3);

	for(;;)
	{
		swiWaitForVBlank();
//		fifoSendValue32(12,mmLayerMain.row);
	}
}
