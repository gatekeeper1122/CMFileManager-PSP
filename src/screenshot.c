#include <oslib/oslib.h>
#include <psprtc.h>

#include "fs.h"
#include "utils.h"

static int num = 0;

static int Screenshot_GenFilename(int number, char *filename) {
	int ret = 0;
	pspTime time;

	if (R_FAILED(ret = sceRtcGetCurrentClockLocalTime(&time)))
		return ret;

	if (!(FS_DirExists("ms0:/PSP/SCREENSHOT/")))
		FS_MakeDir("ms0:/PSP/SCREENSHOT/");
	
	sprintf(filename, "ms0:/PSP/SCREENSHOT/Screenshot_%02d%02d%02d-%i.png", time.year, time.month, time.day, num);
	return 0;
}

void Screenshot_Capture(void) {
	static char filename[256];

	sprintf(filename, "%s", "screenshot");
	Screenshot_GenFilename(num, filename);

	while (FS_FileExists(filename)) {
		num++;
		Screenshot_GenFilename(num, filename);
	}

	oslWriteImageFilePNG(OSL_SECONDARY_BUFFER, filename, 0);
	num++;
}