/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef DXL_FLAGS_H
#define DXL_FLAGS_H

/*
 * This header has declarations for dxl_flags related values
 */

/* Flags (within the context) for handling local and/or management connections */
#define DXL_FLAG_LOCAL    0x01
#define DXL_FLAG_ADMIN    0x02
#define DXL_FLAG_OPS      0x04
#define DXL_FLAG_MANAGED  0x80

#define DXL_LOCAL_ID   "local"
#define DXL_ADMIN_ID   "admin"

#endif /* DXL_FLAGS_H */
