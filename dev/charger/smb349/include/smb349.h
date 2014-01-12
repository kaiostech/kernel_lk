/*
 * smb349.h
 *
 * Copyright (C) Amazon Technologies Inc. All rights reserved.
 * Donald Chan (hoiho@lab126.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#if defined(CONFIG_SMB349)

/* SMB349 registers */
#define SMB349_COMMAND_REG_A  (0x30)
#define SMB349_COMMAND_REG_B  (0x31)
#define SMB349_COMMAND_REG_C  (0x33)
#define SMB349_STATUS_REG_A	  (0x3b)
#define SMB349_STATUS_REG_B	  (0x3c)
#define SMB349_STATUS_REG_C	  (0x3d)
#define SMB349_STATUS_REG_D	  (0x3e)
#define SMB349_STATUS_REG_E	  (0x3f)

#define SMB349_APSD_RESULT_NONE    (0x0)  /* None*/
#define SMB349_APSD_RESULT_OTHER   (0x1)  /* Other charging port */
#define SMB349_APSD_RESULT_SDP     (0x2)  /* Standard downstream port */
#define SMB349_APSD_RESULT_DCP     (0x4)  /* Dedicated Charging Port (Wall Adapter) */
#define SMB349_APSD_RESULT_CDP     (0x8)  /* Charging downstream port */
#define SMB349_APSD_RESULT_ACA_A   (0x10) /* ACA-A charging port */
#define SMB349_APSD_RESULT_ACA_B   (0x20) /* ACA-B charging port */
#define SMB349_APSD_RESULT_ACA_C   (0x40) /* ACA-C charging port */
#define SMB349_APSD_RESULT_ACA     (0x80) /* ACA charging port */

#define SMB349_CHARGING_STATUS(value)	(((value) >> 1) & 0x3)

#define SMB349_CHARGING_STATUS_NOT_CHARGING	(0)
#define SMB349_CHARGING_STATUS_PRE_CHARGING	(1)
#define SMB349_CHARGING_STATUS_FAST_CHARGING	(2)
#define SMB349_CHARGING_STATUS_TAPER_CHARGING	(3)
#define SMB349_CHARGING_STATUS_UNKNOWN		(4)

#define SMB349_USB1_5_HC_MODE(value)	(((value) >> 5) & 0x3)

#define SMB349_HC_MODE			(0)
#define SMB349_USB1_MODE		(1)
#define SMB349_USB5_MODE		(2)

#define SMB349_IS_AICL_DONE(value)	((value) & (1 << 4))

#define SMB349_AICL_RESULT(value)	((value) & 0xf)

#endif
