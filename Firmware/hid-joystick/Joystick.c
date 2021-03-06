/*
             LUFA Library
     Copyright (C) Dean Camera, 2019.

  dean [at] fourwalledcubicle [dot] com
           www.lufa-lib.org
*/

/*
  Copyright 2019  Dean Camera (dean [at] fourwalledcubicle [dot] com)

  Permission to use, copy, modify, distribute, and sell this
  software and its documentation for any purpose is hereby granted
  without fee, provided that the above copyright notice appear in
  all copies and that both that the copyright notice and this
  permission notice and warranty disclaimer appear in supporting
  documentation, and that the name of the author not be used in
  advertising or publicity pertaining to distribution of the
  software without specific, written prior permission.

  The author disclaims all warranties with regard to this
  software, including all implied warranties of merchantability
  and fitness.  In no event shall the author be liable for any
  special, indirect or consequential damages or any damages
  whatsoever resulting from loss of use, data or profits, whether
  in an action of contract, negligence or other tortious action,
  arising out of or in connection with the use or performance of
  this software.
*/

#include "Joystick.h"

/** Buffer to hold the previously generated HID report, for comparison purposes inside the HID class driver. */
static uint8_t PrevJoystickHIDReportBuffer[sizeof(DualShockState)];

/** LUFA HID Class driver interface configuration and state information. This structure is
 *  passed to all HID Class driver functions, so that multiple instances of the same class
 *  within a device can be differentiated from one another.
 */
USB_ClassInfo_HID_Device_t Joystick_HID_Interface = {
		.Config = {
				.InterfaceNumber = INTERFACE_ID_Joystick,
				.ReportINEndpoint = {
						.Address = JOYSTICK_EPADDR,
						.Size = JOYSTICK_EPSIZE,
						.Banks = 1,
					},
				.PrevReportINBuffer = PrevJoystickHIDReportBuffer,
				.PrevReportINBufferSize = sizeof(PrevJoystickHIDReportBuffer),
		},
};

// 2 SNES Gamepad bytes, 7 DualShock bytes, 1 byte of padding
uint8_t data[10];
uint8_t dataCounter;

int main(void) {
	SetupHardware();
	GlobalInterruptEnable();

	for (;;) {
		HID_Device_USBTask(&Joystick_HID_Interface);
		USB_USBTask();

        // Read data on serial connection from the main MCU
        if (Serial_IsCharReceived()) {
            LEDs_TurnOnLEDs(LEDS_LED1);

            if(dataCounter == 10)
                dataCounter = 0;
            data[dataCounter] = Serial_ReceiveByte();
            dataCounter++;
        }
	}
}

void SetupHardware(void) {
	/* Disable watchdog if enabled by bootloader/fuses */
	MCUSR &= ~(1 << WDRF);
	wdt_disable();

	/* Disable clock division */
	clock_prescale_set(clock_div_1);

	/* Hardware Initialization */
	LEDs_Init();
    Serial_Init(115200, true);
	USB_Init();
}

void EVENT_USB_Device_Connect(void)
{
}

void EVENT_USB_Device_Disconnect(void) {
}

void EVENT_USB_Device_ConfigurationChanged(void) {
	HID_Device_ConfigureEndpoints(&Joystick_HID_Interface);
	USB_Device_EnableSOFEvents();
}

void EVENT_USB_Device_ControlRequest(void) {
	HID_Device_ProcessControlRequest(&Joystick_HID_Interface);
}

void EVENT_USB_Device_StartOfFrame(void) {
	HID_Device_MillisecondElapsed(&Joystick_HID_Interface);
}

/** HID class driver callback function for the processing of HID reports from the host.
 *
 *  \param[in] HIDInterfaceInfo  Pointer to the HID class interface configuration structure being referenced
 *  \param[in] ReportID    Report ID of the received report from the host
 *  \param[in] ReportType  The type of report that the host has sent, either REPORT_ITEM_TYPE_Out or REPORT_ITEM_TYPE_Feature
 *  \param[in] ReportData  Pointer to a buffer where the created report has been stored
 *  \param[in] ReportSize  Size in bytes of the received HID report
 */
uint8_t lastReportId = 0;
bool CALLBACK_HID_Device_CreateHIDReport(USB_ClassInfo_HID_Device_t *const HIDInterfaceInfo, uint8_t *const ReportID,
                                         const uint8_t ReportType, void *ReportData, uint16_t *const ReportSize) {
    // Wait for the two values to be filled in
    if (dataCounter != 10)
        return false;

    LEDs_TurnOffLEDs(LEDS_LED1);

    // Seems that we don't receive ReportID, so let's alternate between the two
    if (*ReportID == 0) {
        if (lastReportId == 0x01)
            lastReportId = 0x02;
        else
            lastReportId = 0x01;
        *ReportID = lastReportId;
    } else {
        lastReportId = *ReportID;
    }

    // SNES Gamepad
    if (*ReportID == 0x01) {
        SnesGamepadState *state = (SnesGamepadState *)ReportData;

        state->direction = data[0];
        state->buttons = data[1];

        *ReportSize = sizeof(SnesGamepadState);
        return true;
    // DualShock
    } else if (*ReportID == 0x02) {
        DualShockState *state = (DualShockState *)ReportData;

        state->direction = data[2];
        state->buttons[0] = data[3];
        state->buttons[1] = data[4];
        // Convert sticks data from 0 to 255 into -100 to 100
        int8_t lx = data[5] - 0x80;
        if (lx >= 0x1c)
            lx -= 0x1c;
        else if (lx <= -0x1c)
            lx += 0x1c;
        state->leftStick[0] = lx;

        int8_t ly = data[6] - 0x80;
        if (ly >= 0x1c)
            ly -= 0x1c;
        else if (ly <= -0x1c)
            ly += 0x1c;
        state->leftStick[1] = ly;

        int8_t rx = data[7] - 0x80;
        if (rx >= 0x1c)
            rx -= 0x1c;
        else if (rx <= -0x1c)
            rx += 0x1c;
        state->rightStick[0] = rx;

        int8_t ry = data[8] - 0x80;
        if (ry >= 0x1c)
            ry -= 0x1c;
        else if (ry <= -0x1c)
            ry += 0x1c;
        state->rightStick[1] = ry;

        *ReportSize = sizeof(DualShockState);
        return true;
    }

	return false;
}

void CALLBACK_HID_Device_ProcessHIDReport(USB_ClassInfo_HID_Device_t* const HIDInterfaceInfo, const uint8_t ReportID,
                                          const uint8_t ReportType, const void* ReportData, const uint16_t ReportSize) {
}

