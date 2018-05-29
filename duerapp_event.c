
#include <termios.h>

#include "duerapp_event.h"
#include "duerapp_record.h"

void duerapp_event_loop(void)
{
	bool loop = true;
	char event;
	struct termios kbd_ops, kbd_bak;

	tcgetattr(STDIN_FILENO, &kbd_ops);
	memcpy(&kbd_bak, &kbd_ops, sizeof(kbd_bak));
	kbd_ops.c_lflag &= ~ICANON;
	tcsetattr(STDIN_FILENO, TCSANOW, &kbd_ops);

	while (loop)
	{
		printf("duerapp# ");
		fflush(STDIN_FILENO);
		int ret = read(STDIN_FILENO, &event, 1);
		if (ret <= 0) {
			DUER_LOGE("fgets stdin failed!!");
			continue;
		}

		switch(event) {
		case KBDEVT_RECORD:
			duerapp_record_capture_onoff();
			break;
		case KBDEVT_QUIT:
			printf("\n");
			loop = false;
			break;
		}
	}

	tcsetattr(STDIN_FILENO, TCSANOW, &kbd_bak);
}

