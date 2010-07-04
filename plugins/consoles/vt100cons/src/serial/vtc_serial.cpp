// vtc_serial.cpp
// 
// Copyright (c) 2007 - 2010 Accenture. All rights reserved.
// This component and the accompanying materials are made available
// under the terms of the "Eclipse Public License v1.0"
// which accompanies this distribution, and is available
// at the URL "http://www.eclipse.org/legal/epl-v10.html".
// 
// Initial Contributors:
// Accenture - Initial contribution
//

#include <fshell/common.mmh>
#include "vtc_serial.h"

const TInt KMaxWriteLength = 2 * 1024; // Some serial drivers fail with KErrNoMemory if writes are too big, so they have to be broken up.

#define LEAVE_IF_ERROR_ALLOW_ALREADY_EXISTS(_x) { TInt _err = _x; if ((_err < 0) && (_err != KErrAlreadyExists)) { User::Leave(_err); } }

EXPORT_C CVtcSerialConsole::CVtcSerialConsole()
	{
	}

EXPORT_C CVtcSerialConsole::~CVtcSerialConsole()
	{
	iCommPort.Close();
	iCommServ.Close();
	}

TInt CVtcSerialConsole::ReadConfig(const TDesC& aConfigDes, TPortConfig& aConfig)
	{
	_LIT(KKeywordPdd, "pdd");
	_LIT(KKeywordLdd, "ldd");
	_LIT(KKeywordCsy, "csy");
	_LIT(KKeywordPort, "port");
	_LIT(KKeywordRate, "rate");

	TBool keywordFound(EFalse);
	TLex lex(aConfigDes);
	while (!lex.Eos())
		{
		TPtrC keyword;
		TPtrC value;
		TInt err = ReadKeywordValuePair(lex, keyword, value);
		if (err != KErrNone)
			{
			break;
			}

		if (keyword == KKeywordPdd)
			{
			aConfig.iPdd.Set(value);
			keywordFound = ETrue;
			}
		else if (keyword == KKeywordLdd)
			{
			aConfig.iLdd.Set(value);
			keywordFound = ETrue;
			}
		else if (keyword == KKeywordCsy)
			{
			aConfig.iCsy.Set(value);
			keywordFound = ETrue;
			}
		else if (keyword == KKeywordPort)
			{
			aConfig.iPort.Set(value);
			keywordFound = ETrue;
			}
		else if (keyword == KKeywordRate)
			{
			TLex lex(value);
			TUint rate;
			if (lex.Val(rate) == KErrNone)
				{
				switch (rate)
					{
				case 115200:
					aConfig.iRate = EBps115200; break;
				case 9600:
					aConfig.iRate = EBps9600; break;
				case 19200:
					aConfig.iRate = EBps19200; break;
				case 57600:
					aConfig.iRate = EBps57600; break;
				default:
					break;
					}
				}
			keywordFound = ETrue;
			}
		}

	if (!keywordFound)
		{
		// Treat unrecognised string as a port name (to preserve backwards compatibility with earlier releases).
		aConfig.iPort.Set(aConfigDes);
		}

	return KErrNone;
	}

EXPORT_C void CVtcSerialConsole::ConstructL(const TDesC& aTitle)
	{
	TPortConfig portConfig;
	User::LeaveIfError(ReadConfig(aTitle, portConfig));
	if (portConfig.iPort.Length() == 0)
		{
		User::Leave(KErrArgument);
		}
	if (portConfig.iPdd.Length())
		{
		LEAVE_IF_ERROR_ALLOW_ALREADY_EXISTS(User::LoadPhysicalDevice(portConfig.iPdd));
		}
	else
		{
		// If not specified, assume they probably intended the standard ones (but don't error if they fail, since the user might actually know what they're doing
#ifdef __WINS__
		User::LoadPhysicalDevice(_L("ecdrv"));
#else
		User::LoadPhysicalDevice(_L("euart1"));
#endif
		}

	if (portConfig.iLdd.Length())
		{
		LEAVE_IF_ERROR_ALLOW_ALREADY_EXISTS(User::LoadLogicalDevice(portConfig.iLdd));
		}
	else
		{
		User::LoadLogicalDevice(_L("ecomm"));
		}

	User::LeaveIfError(iCommServ.Connect());
	if (portConfig.iCsy.Length())
		{
		LEAVE_IF_ERROR_ALLOW_ALREADY_EXISTS(iCommServ.LoadCommModule(portConfig.iCsy));
		}
	else
		{
		iCommServ.LoadCommModule(_L("ecuart"));
		}
	User::LeaveIfError(iCommPort.Open(iCommServ, portConfig.iPort, ECommExclusive));

#ifdef __WINS__
	// For BC reasons, we always set rate on WINS
	if (portConfig.iRate == EBpsAutobaud) portConfig.iRate = EBps115200;
#endif

	if (portConfig.iRate != EBpsAutobaud)
		{
		TCommConfig cfg;
		cfg().iRate = portConfig.iRate;
		cfg().iDataBits = EData8;
		cfg().iStopBits = EStop1;
		cfg().iParity = EParityNone;
		cfg().iHandshake = 0;
		cfg().iParityError = KConfigParityErrorFail;
		cfg().iFifo = EFifoEnable;
		cfg().iSpecialRate = 0;
		cfg().iTerminatorCount = 0;
		cfg().iSIREnable = ESIRDisable;
		User::LeaveIfError(iCommPort.SetConfig(cfg));
		}
	
	User::LeaveIfError(iCommPort.ResetBuffers());
	CVtcConsoleBase::ConstructL(aTitle);
	}
	
EXPORT_C TInt CVtcSerialConsole::Output(const TDesC8& aDes)
	{
	TRequestStatus status;
	TInt offset = 0;
	TInt err = KErrNone;
	while ((offset < aDes.Length()) && (err == KErrNone))
		{
		const TInt bytesToWrite = Min(aDes.Length() - offset, KMaxWriteLength);
		do
			{
			iCommPort.Write(status, aDes.Mid(offset, bytesToWrite));
			User::WaitForRequest(status);
			err = status.Int();
			if (err == KErrNone)
				{
				offset += bytesToWrite;
				}
			else if (err == KErrNoMemory)	// Necessary because of nasty behaviour in loopback.csy where it returns KErrNoMemory when its buffer is full, rather than simply delaying completion of the request until there is enough space.
				{
				User::After(200000);
				}
			}
#ifdef FSHELL_PLATFORM_OPP // Temporary OPP specific change - the drivers for the mid-sized prototype currently complete requests with KErrAbort just before power management sends the device to sleep.
		while ((err == KErrNoMemory) || (err == KErrAbort));
#else
		while (err == KErrNoMemory);
#endif
		}

	return err;
	}

EXPORT_C void CVtcSerialConsole::Input(TDes8& aDes, TRequestStatus& aStatus)
	{
	iCommPort.ReadOneOrMore(aStatus, aDes);
	}

EXPORT_C void CVtcSerialConsole::CancelInput(TRequestStatus&)
	{
	iCommPort.ReadCancel();
	}
