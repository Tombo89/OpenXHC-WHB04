/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : usbd_custom_hid_if.c
  * @version        : v2.0_Cube
  * @brief          : USB Device Custom HID interface file.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "usbd_custom_hid_if.h"

/* USER CODE BEGIN INCLUDE */
#include <string.h>
#include "usb_device.h"
#include "usbd_customhid.h"  // deklariert USBD_CUSTOM_HID_ReceivePacket()
#include "usbd_core.h"
#include "xhc_receive.h"
#include "xhc_main.h"

#ifndef __USB_DEVICE__H
extern USBD_HandleTypeDef hUsbDeviceFS;
#endif

#ifndef USBD_CUSTOMHID_OUTREPORT_BUF_SIZE
#define USBD_CUSTOMHID_OUTREPORT_BUF_SIZE 64u
#endif
/* USER CODE END INCLUDE */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/
#define XHC_OUT_MAX_LEN   64u            // Größe eines Host->Device Reports
#define XHC_RX_RING_SIZE  8u             // Anzahl gepufferter Reports (2..16)
#define XHC_FEAT_MAX_LEN  USBD_CUSTOMHID_OUTREPORT_BUF_SIZE

// Global counter nur für Display-Debug
volatile uint32_t debug_setreport_calls = 0;
volatile uint32_t debug_outevent_calls = 0;


typedef struct {
    uint16_t len;
    uint8_t  data[XHC_OUT_MAX_LEN];
} xhc_rx_item_t;

static volatile uint16_t   rx_head = 0;     // schreibt der USB-IRQ
static volatile uint16_t   rx_tail = 0;     // liest die Anwendung
static volatile uint32_t   rx_dropped = 0;  // Statistik: überlaufene Pakete
static xhc_rx_item_t       rx_ring[XHC_RX_RING_SIZE];

/* Hilfs-Makros */
#define RING_NEXT(i)  (uint16_t)(((i) + 1u) % XHC_RX_RING_SIZE)
#define RING_EMPTY()  (rx_head == rx_tail)
#define RING_FULL()   (RING_NEXT(rx_head) == rx_tail)
/* USER CODE END PV */

/** @addtogroup STM32_USB_OTG_DEVICE_LIBRARY
  * @brief Usb device.
  * @{
  */

/** @addtogroup USBD_CUSTOM_HID
  * @{
  */

/** @defgroup USBD_CUSTOM_HID_Private_TypesDefinitions USBD_CUSTOM_HID_Private_TypesDefinitions
  * @brief Private types.
  * @{
  */

/* USER CODE BEGIN PRIVATE_TYPES */
/* --------- API für Anwendung / Debug --------- */

uint8_t XHC_RX_TryPop(uint8_t *dst, uint16_t *io_len)
{
    if (RING_EMPTY()) return 0;

    uint16_t idx = rx_tail;
    uint16_t n   = rx_ring[idx].len;

    if (dst && io_len && *io_len >= n) {
        memcpy(dst, rx_ring[idx].data, n);
        *io_len = n;
        rx_tail = RING_NEXT(rx_tail);
        return 1;
    }

    if (io_len) {
        *io_len = n;   // teilt dem Aufrufer mit, wie groß er sein muss
    }
    return 0;
}

uint32_t XHC_RX_Count(void){ return (rx_head>=rx_tail)? (rx_head-rx_tail):(XHC_RX_RING_SIZE-(rx_tail-rx_head)); }
uint32_t XHC_RX_Dropped(void){ return rx_dropped; }

/* Helper: OUT-EP nach Empfang wieder scharf schalten */
static inline void XHC_Push_(const uint8_t *buf, uint16_t len)
{
    if (len > XHC_FEAT_MAX_LEN) len = XHC_FEAT_MAX_LEN;

    if (!RING_FULL()) {
        uint16_t idx = rx_head;
        rx_ring[idx].len = len;
        memcpy(rx_ring[idx].data, buf, len);
        rx_head = RING_NEXT(rx_head);
    } else {
        rx_dropped++;
    }
}
/* USER CODE END PRIVATE_TYPES */

/**
  * @}
  */

/** @defgroup USBD_CUSTOM_HID_Private_Defines USBD_CUSTOM_HID_Private_Defines
  * @brief Private defines.
  * @{
  */

/* USER CODE BEGIN PRIVATE_DEFINES */

/* USER CODE END PRIVATE_DEFINES */

/**
  * @}
  */

/** @defgroup USBD_CUSTOM_HID_Private_Macros USBD_CUSTOM_HID_Private_Macros
  * @brief Private macros.
  * @{
  */

/* USER CODE BEGIN PRIVATE_MACRO */

/* USER CODE END PRIVATE_MACRO */

/**
  * @}
  */

/** @defgroup USBD_CUSTOM_HID_Private_Variables USBD_CUSTOM_HID_Private_Variables
  * @brief Private variables.
  * @{
  */

/** Usb HID report descriptor. */
__ALIGN_BEGIN static uint8_t CUSTOM_HID_ReportDesc_FS[USBD_CUSTOM_HID_REPORT_DESC_SIZE] __ALIGN_END =
{
  /* USER CODE BEGIN 0 */
	    0x06,0x00,0xFF, 	        /* Usage Page (Vendor-Defined 1) */
	    0x09,0x01, 			/* Usage (Vendor-Defined 1) */
	    0xA1,0x01, 			/* Collection (Application) */
	    0x85,0x04, 				/* Report ID (4) */
	    0x09,0x01, 				/* Usage (Vendor-Defined 1) */
	    0x15,0x00, 				/* Logical Minimum (0) */
	    0x26,0xFF,0x00, 		        /* Logical Maximum (255) */
	    0x95,0x05, 				/* Report Count (5) */
	    0x75,0x08, 				/* Report Size (8) */
	    0x81,0x02, 				/* Input (Data,Var,Abs,NWrp,Lin,Pref,NNul,Bit) */

		0xC0, 			/* End Collection */

	    0x06,0x00,0xFF, 	        /* Usage Page (Vendor-Defined 1) */
	    0x09,0x01, 			/* Usage (Vendor-Defined 1) */
	    0xA1,0x01,	/* Collection (Application) */

	    0x85,0x06, 				/* Report ID (6) */
	    0x09,0x01, 				/* Usage (Vendor-Defined 1) */
	    0x15,0x00, 				/* Logical Minimum (0) */
	    0x26,0xFF,0x00, 		        /* Logical Maximum (255) */
	    0x95,0x07, 				/* Report Count (7) */
	    0x75,0x08, 				/* Report Size (8) */
	    0xB1,0x06, 				/* Feature (Data,Var,Rel,NWrp,Lin,Pref,NNul,NVol,Bit) */
  /* USER CODE END 0 */
  0xC0    /*     END_COLLECTION	             */
};

/* USER CODE BEGIN PRIVATE_VARIABLES */

/* USER CODE END PRIVATE_VARIABLES */

/**
  * @}
  */

/** @defgroup USBD_CUSTOM_HID_Exported_Variables USBD_CUSTOM_HID_Exported_Variables
  * @brief Public variables.
  * @{
  */
extern USBD_HandleTypeDef hUsbDeviceFS;

/* USER CODE BEGIN EXPORTED_VARIABLES */

/* USER CODE END EXPORTED_VARIABLES */
/**
  * @}
  */

/** @defgroup USBD_CUSTOM_HID_Private_FunctionPrototypes USBD_CUSTOM_HID_Private_FunctionPrototypes
  * @brief Private functions declaration.
  * @{
  */

static int8_t CUSTOM_HID_Init_FS(void);
static int8_t CUSTOM_HID_DeInit_FS(void);
static int8_t CUSTOM_HID_OutEvent_FS(uint8_t event_idx, uint8_t state);
static int8_t CUSTOM_HID_SetReport_FS(uint8_t *report, uint16_t len);

/**
  * @}
  */

USBD_CUSTOM_HID_ItfTypeDef USBD_CustomHID_fops_FS =
{
  CUSTOM_HID_ReportDesc_FS,
  CUSTOM_HID_Init_FS,
  CUSTOM_HID_DeInit_FS,
  CUSTOM_HID_OutEvent_FS,
  CUSTOM_HID_SetReport_FS,
};

/** @defgroup USBD_CUSTOM_HID_Private_Functions USBD_CUSTOM_HID_Private_Functions
  * @brief Private functions.
  * @{
  */

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Initializes the CUSTOM HID media low layer
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CUSTOM_HID_Init_FS(void)
{
  /* USER CODE BEGIN 4 */

  return (USBD_OK);
  /* USER CODE END 4 */
}

/**
  * @brief  DeInitializes the CUSTOM HID media low layer
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CUSTOM_HID_DeInit_FS(void)
{
  /* USER CODE BEGIN 5 */
  return (USBD_OK);
  /* USER CODE END 5 */
}

/**
  * @brief  Manage the CUSTOM HID class events
  * @param  event_idx: Event index
  * @param  state: Event state
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CUSTOM_HID_OutEvent_FS(uint8_t event_idx, uint8_t state)
{
  /* USER CODE BEGIN 6 */
	debug_outevent_calls++;

	(void)event_idx;
	    (void)state;

	    USBD_CUSTOM_HID_HandleTypeDef *hhid =
	        (USBD_CUSTOM_HID_HandleTypeDef*)hUsbDeviceFS.pClassData;

	    if (!hhid) {
	        return (int8_t)USBD_OK;
	    }

	    /* In deinem Report-Descriptor ist:
	         - Feature Report ID 0x06 mit 7 Bytes Payload => total 8 Byte inkl. ReportID
	       Viele Stacks liefern in hhid->Report_buf genau diese 8 Byte.
	       Falls mal mehr ankommt, caps durch XHC_FEAT_MAX_LEN. */
	    uint16_t len = USBD_CUSTOMHID_OUTREPORT_BUF_SIZE;
	    if (len > XHC_FEAT_MAX_LEN) len = XHC_FEAT_MAX_LEN;

	    /* Optional: Wenn Report-ID 0x06 erzwungen werden soll, Länge hart auf 8 setzen */
	    if (hhid->Report_buf[0] == 0x06) {
	        len = (8u <= XHC_FEAT_MAX_LEN) ? 8u : XHC_FEAT_MAX_LEN;
	    }

	    XHC_Push_(hhid->Report_buf, len);
  return (USBD_OK);
  /* USER CODE END 6 */
}

/* USER CODE BEGIN 7 */
/**
  * @brief  Send the report to the Host
  * @param  report: The report to be sent
  * @param  len: The report length
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
/*
static int8_t USBD_CUSTOM_HID_SendReport_FS(uint8_t *report, uint16_t len)
{
  return USBD_CUSTOM_HID_SendReport(&hUsbDeviceFS, report, len);
}
*/
/* USER CODE END 7 */

/* USER CODE BEGIN PRIVATE_FUNCTIONS_IMPLEMENTATION */

static int8_t CUSTOM_HID_SetReport_FS(uint8_t *report, uint16_t len)
{
  /* USER CODE BEGIN 7 */
	  debug_setreport_calls++;  // Zähler erhöhen
  /* XHC HB04 Integration */
  if (len >= 8 && report[0] == 0x06)
  {
    /* Report ID 0x06 - Host→Device Kommunikation für XHC */
    xhc_recv(&report[1]);  // Überspringe Report ID
    return USBD_OK;
  }

  /* USER CODE END 7 */
  return (USBD_OK);
}

/* USER CODE END PRIVATE_FUNCTIONS_IMPLEMENTATION */
/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */
