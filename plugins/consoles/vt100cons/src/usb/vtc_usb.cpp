// vtc_usb.cpp
// 
// Copyright (c) 2008 - 2010 Accenture. All rights reserved.
// This component and the accompanying materials are made available
// under the terms of the "Eclipse Public License v1.0"
// which accompanies this distribution, and is available
// at the URL "http://www.eclipse.org/legal/epl-v10.html".
// 
// Initial Contributors:
// Accenture - Initial contribution
//

#include <e32std.h>
#include <fshell/consoleextensions.h>
#include "vtc_usb.h"
#include <usbclassuids.h>

EXPORT_C TAny* NewConsole()
	{
	return new CUsbConsole;
	}
	
//______________________________________________________________________________
//						CUsbConsole
CUsbConsole::CUsbConsole()
	{
	}

CUsbConsole::~CUsbConsole()
	{
	iUsb.Close();
	}
	
void CUsbConsole::ConstructL(const TDesC& aTitle)
	{
	User::LeaveIfError(iUsb.Connect());
	
	TRequestStatus stat;
	// assume USB device already started
	/*iUsb.Start(stat);
	User::WaitForRequest(stat);
	// KErrAccessDenied returned if already started;
	if (stat.Int()!=KErrAccessDenied)
		{
		Message(EError, KUsbError, stat.Int(), 1);
		User::LeaveIfError(stat.Int());
		}*/

	// Wait for an enumeration that supports ACM (this is so that if the device defaulted to say mass storage and was then reconfigured to a personality with ACM, we wait for the ACM reconfiguration
	TBool gotAcm = EFalse;
	while (!gotAcm)
		{
		TUsbDeviceState usbState;
		User::LeaveIfError(iUsb.GetDeviceState(usbState));
		if (usbState & EUsbDeviceStateConfigured)
			{
			// Check if we have ACM
			TInt currentPersonality;
			User::LeaveIfError(iUsb.GetCurrentPersonalityId(currentPersonality));
			User::LeaveIfError(iUsb.ClassSupported(currentPersonality, KECACMUid, gotAcm));
			_LIT(KGotIt, "Current USB personality has ACM, proceeding");
			_LIT(KNotGotIt, "Current USB personality doesn't have ACM, waiting for re-enumeration");
			if (gotAcm) Message(EInformation, KGotIt);
			else Message(EInformation, KNotGotIt);
			}

		if (!gotAcm)
			{
			// We're not enumerated, or we are but don't have ACM. So wait for a (re-)enumeration
			_LIT(KWaitingForEnumeration, "Waiting for USB enumeration (please connect USB cable)");
			Message(EInformation, KWaitingForEnumeration);
			iUsb.DeviceStateNotification(EUsbDeviceStateConfigured, usbState, stat);
			User::WaitForRequest(stat);
			if (stat.Int() != KErrNone)
				{
				_LIT(KUsbError, "Error configuring USB: %d");
				Message(EError, KUsbError, stat.Int());
				User::Leave(stat.Int());
				}
			_LIT(KUsbEnumerated, "USB cable connected.");
			Message(EInformation, KUsbEnumerated);
			}
		}
	
	// Run the preamble script, if we have one. Because iosrv is multithreaded this shouldn't cause a deadlock so long as we use a different console (in this case, nullcons)
	_LIT(KPreamble, "--console nullcons vt100usbcons_preamble");
	RProcess preamble;
	TInt err = preamble.Create(_L("fshell.exe"), KPreamble);
	if (err == KErrNone)
		{
		preamble.Logon(stat);
		if (stat == KRequestPending)
			{
			preamble.Resume();
			User::WaitForRequest(stat);
			}
		err = stat.Int();
		preamble.Close();
		}

	if (err == KErrNone)
		{
		Message(EInformation, _L("Preamble script ran ok"));
		}
	else if (err != KErrNotFound)
		{
		Message(EInformation, _L("Preamble script failed with %d"), err);
		}

	//TODO should we ensure that the port passed in here is an ACM::%s port?
	Message(EInformation, _L("Opening %S"), &aTitle);
	CVtcSerialConsole::ConstructL(aTitle);
	}
