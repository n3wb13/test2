#include "globals.h"
#include "reader/serial.h"

#include "simples.h"
#include "log.h"

/* CT-API */
#include "ctapi.h"
#include "ctbcs.h"
#include "defines.h"

#define CTAPI_CTN 1

int reader_serial_irdeto_mode;		// UGLY : to be removed
int reader_serial_card_detect;		// UGLY : to be removed
int reader_serial_mhz;			// UGLY : to be removed

static ushort reader_serial_get_reader_type(struct s_reader *reader)
{
	ushort reader_type = RTYP_STD;
#ifdef TUXBOX
	struct stat sb;
#endif

	switch (reader->type) {
		case R_MOUSE :
			reader_type = RTYP_STD;
#ifdef TUXBOX
			if (!stat(reader->device, &sb)) {
				if (S_ISCHR(sb.st_mode)) {
					int dev_major, dev_minor;
					dev_major = major(sb.st_rdev);
					dev_minor = minor(sb.st_rdev);

					if (cs_hw == CS_HW_DBOX2 && (dev_major == 4 || dev_major == 5)) {
						switch (dev_minor & 0x3F) {
							case 0 :
								reader_type = RTYP_DB2COM1;
								break;
							case 1 :
								reader_type = RTYP_DB2COM2;
								break;
						}
					}

					cs_debug("device is major: %d, minor: %d, reader_type=%d", dev_major, dev_minor, reader_type);
				}
			}
#endif
			break;

		case R_SMART :
			reader_type = RTYP_SMART;
			break;

		case R_INTERN :
			reader_type = RTYP_SCI;
			break;
	}

	return reader_type;
}

static int reader_serial_cmd2api(uchar dad, uchar *cmd, ushort cmd_size, uchar *result, ushort result_max_size, ushort *result_size, int dbg)
{
	char ret;
	uchar sad = 2;

	// Set result_size to the size of the result buffer (result_max_size)
	*result_size = result_max_size;

	// Save and Change cs_ptyp
	int cs_ptyp_orig = cs_ptyp;
	cs_ptyp = dbg;

//	cs_ddump(cmd, cmd_size, "send %d bytes to ctapi", cmd_size);

	// Call CT-API
	ret = CT_data(
		CTAPI_CTN,	/* Terminal Number */
		&dad,		/* Destination */
		&sad,		/* Source */
		cmd_size,	/* Length of command */
		cmd,		/* Command/Data Buffer */
		result_size,	/* Length of Response */
		result);	/* Response */

//	cs_ddump(result, *result_size, "received %d bytes from ctapi with ret=%d", *result_size, ret);

	// Restore cs_ptyp
	cs_ptyp = cs_ptyp_orig;

	return ret;
}

static int reader_serial_cmd2reader(uchar *cmd, ushort cmd_size, uchar *result, ushort result_max_size, ushort *result_size)
{
	return reader_serial_cmd2api(1, cmd, cmd_size, result, result_max_size, result_size, D_DEVICE);
}

int reader_serial_cmd2card(uchar *cmd, ushort cmd_size, uchar *result, ushort result_max_size, ushort *result_size)
{
	return reader_serial_cmd2api(0, cmd, cmd_size, result, result_max_size, result_size, D_DEVICE);
}

int reader_serial_init(struct s_reader *reader)
{
	char ret;

	// Set some extern variables to be used by CT-API
	reader_serial_card_detect = reader->detect;
	reader_serial_mhz = reader->mhz;

	// Save and Change cs_ptyp
	int cs_ptyp_orig = cs_ptyp;
	cs_ptyp = D_DEVICE;

	// Lookup Port Number
	ushort reader_type = reader_serial_get_reader_type(reader);
	if ((ret = CT_init(CTAPI_CTN, reader->device, reader_type)) != OK) {
		cs_log("Reader: Cannot open device \"%s\" (%d) !", reader->device, ret);
	}

	// Restore cs_ptyp
	cs_ptyp = cs_ptyp_orig;

	return (ret == OK);
}

int reader_serial_reset()
{
	char ret;
	uchar cmd[5];
	uchar result[260];
	ushort result_size;

	/* Reset CardTerminal */
	cmd[0] = CTBCS_INS_RESET;
	cmd[1] = CTBCS_P2_RESET_GET_ATR;
	cmd[2] = 0x00;

	ret = reader_serial_cmd2reader(cmd, 3, result, sizeof(result), &result_size);
	if (ret != OK) {
		cs_log("Reader: Error resetting terminal card (%d, %s) !", ret, cs_hexdump(1, result, result_size));
		return 0;
	}

	return 1;
}

int reader_serial_card_is_inserted()
{
	char ret;
	uchar cmd[5];
	uchar result[260];
	ushort result_size;

	/* Get Status of CardTerminal */
	cmd[0] = CTBCS_CLA;
	cmd[1] = CTBCS_INS_STATUS;
	cmd[2] = CTBCS_P1_CT_KERNEL;
	cmd[3] = CTBCS_P2_STATUS_ICC;
	cmd[4] = 0x00;

	ret = reader_serial_cmd2reader(cmd, 5, result, sizeof(result), &result_size);
	if (!(ret == OK && result_size == 3 && result[1] == CTBCS_SW1_OK && result[2] == CTBCS_SW2_OK)) {
		cs_log("Reader: Error getting status of terminal (%d, %s) !", ret, cs_hexdump(1, result, result_size));
		return 0;
	}

	return (result[0] == CTBCS_DATA_STATUS_CARD_CONNECT);
}

int reader_serial_get_atr(uchar *atr, ushort *atr_size)
{
	int i;
	char ret;
	uchar cmd[5];
	uchar result[260];
	ushort result_size;

	/* Try to get ATR from card */
	for (i = 0; i < 3; i++) {
		reader_serial_irdeto_mode = (i % 2);

		/* Request ICC */
		cmd[0] = CTBCS_CLA;
		cmd[1] = CTBCS_INS_REQUEST;
		cmd[2] = CTBCS_P1_INTERFACE1;
		cmd[3] = CTBCS_P2_REQUEST_GET_ATR;
		cmd[4] = 0x00;

		ret = reader_serial_cmd2reader(cmd, 5, result, sizeof(result), &result_size);
		if (ret == OK && result_size > 2 && result[result_size - 2] == CTBCS_SW1_REQUEST_ASYNC_OK && result[result_size -1] == CTBCS_SW2_REQUEST_ASYNC_OK) {
			/* Store Answer to Reset */
			*atr_size = result_size - 2;
			memcpy(atr, result, *atr_size);

			/* All is ok */
			return 1;
		} else {
			cs_log("Reader: Error getting ATR from card (%d, %s) !", ret, cs_hexdump(1, result, result_size));
			cs_sleepms(500);
		}
	}

	/* Cannot get the ATR */
	return 0;
}
