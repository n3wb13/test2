/*
    ct_slot.h
    Card Terminal Slot handling definitions

    This file is part of the Unix driver for Towitoko smartcard readers
    Copyright (C) 2000 Carlos Prados <cprados@yahoo.com>

    This version is modified by doz21 to work in a special manner ;)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef _CT_SLOT_
#define _CT_SLOT_

#include "defines.h"
#include "apdu.h"
#include "ctapi.h"
#include "ctbcs.h"


/* 
 * Exported constats definition
 */

/* Type of protocol and ICC */
#define CT_SLOT_PROTOCOL_T0	0
#define CT_SLOT_PROTOCOL_T1	1
#define CT_SLOT_PROTOCOL_T14	14
#define CT_SLOT_PROTOCOL_SYNC	16	
#define CT_SLOT_ICC_ASYNC	0
#define CT_SLOT_ICC_SYNC	1
#define CT_SLOT_NULL		-1

/*
 * Exported datatypes definition 
 */

typedef struct
{
  int icc_type;		/* Type of ICC */
}
CT_Slot;

/*
 * Exported functions declaration
 */

/* Cretate a new CT_Slot */
extern CT_Slot *
CT_Slot_New ();

/* Intialice a CT_Slot */
extern char 
CT_Slot_Init ();

/* Check for card inserted */
extern char
CT_Slot_Check (bool * card, bool * change);

/* Probe ICC type and protocol */
extern char
CT_Slot_Probe (CT_Slot * slot, BYTE * userdata, unsigned length);

/* Release status information */
extern char
CT_Slot_Release (CT_Slot * slot);

/* Return ICC type */
extern int
CT_Slot_GetICCType (CT_Slot * slot);

/* Get answer to reset of the card */
extern void *
CT_Slot_GetAtr (CT_Slot * slot);

/* Close a CT_Slot */
extern char 
CT_Slot_Close (CT_Slot * slot);

/* Delete a CT_Slot */
extern void 
CT_Slot_Delete (CT_Slot * slot);

#endif

