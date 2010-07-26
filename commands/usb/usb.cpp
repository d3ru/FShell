// usb.cpp
// 
// Copyright (c) 2010 Accenture. All rights reserved.
// This component and the accompanying materials are made available
// under the terms of the "Eclipse Public License v1.0"
// which accompanies this distribution, and is available
// at the URL "http://www.eclipse.org/legal/epl-v10.html".
// 
// Initial Contributors:
// Accenture - Initial contribution
//

#include <fshell/ioutils.h>
#include <fshell/common.mmh>
#include <usbman.h>

using namespace IoUtils;

class CCmdUsb : public CCommandBase
	{
public:
	static CCommandBase* NewLC();
	~CCmdUsb();
private:
	CCmdUsb();
private: // From CCommandBase.
	virtual const TDesC& Name() const;
	virtual void DoRunL();
	virtual void ArgumentsL(RCommandArgumentList& aArguments);
	virtual void OptionsL(RCommandOptionList& aOptions);
private:
	RUsb iUsb;
	TInt iPersonality;
	};

EXE_BOILER_PLATE(CCmdUsb)

CCommandBase* CCmdUsb::NewLC()
	{
	CCmdUsb* self = new(ELeave) CCmdUsb();
	CleanupStack::PushL(self);
	self->BaseConstructL();
	return self;
	}

CCmdUsb::~CCmdUsb()
	{
	iUsb.Close();
	}

CCmdUsb::CCmdUsb()
	{
	}

const TDesC& CCmdUsb::Name() const
	{
	_LIT(KName, "usb");	
	return KName;
	}

void CCmdUsb::ArgumentsL(RCommandArgumentList& aArguments)
	{
	aArguments.AppendIntL(iPersonality, _L("personality"));
	}

void CCmdUsb::OptionsL(RCommandOptionList& /*aOptions*/)
	{
	}

#define CASE_LIT(x) case x: { _LIT(KName, #x); return &KName; }

const TDesC* DeviceState(TUsbDeviceState aState)
	{
	switch (aState)
		{
		CASE_LIT(EUsbDeviceStateUndefined);
		CASE_LIT(EUsbDeviceStateDefault);
		CASE_LIT(EUsbDeviceStateAttached);
		CASE_LIT(EUsbDeviceStatePowered);
		CASE_LIT(EUsbDeviceStateConfigured);
		CASE_LIT(EUsbDeviceStateAddress);
		CASE_LIT(EUsbDeviceStateSuspended);
		default:
			{
			_LIT(KUnknown, "?");
			return &KUnknown;
			}
		}
	}

const TDesC* ServiceState(TUsbServiceState aState)
	{
	switch (aState)
		{
		CASE_LIT(EUsbServiceIdle);
		CASE_LIT(EUsbServiceStarting);
		CASE_LIT(EUsbServiceStarted);
		CASE_LIT(EUsbServiceStopping);
		CASE_LIT(EUsbServiceFatalError);
		default:
			{
			_LIT(KUnknown, "?");
			return &KUnknown;
			}
		}
	}

void CCmdUsb::DoRunL()
	{
	LeaveIfErr(iUsb.Connect(), _L("Couldn't connect to RUsb"));

	TUsbDeviceState dev;
	LeaveIfErr(iUsb.GetDeviceState(dev), _L("Couldn't get device state"));
	Printf(_L("Usb Device state = %S (%d)\r\n"), DeviceState(dev), dev);

	TUsbServiceState servicestate;
	LeaveIfErr(iUsb.GetServiceState(servicestate), _L("Couldn't get service state"));
	Printf(_L("Usb Service state = %S (%d)\r\n"), ServiceState(servicestate), servicestate);

	RArray<TInt> ids;
	LeaveIfErr(iUsb.GetPersonalityIds(ids), _L("Couldn't get personalities ids"));
	for (TInt i = 0; i < ids.Count(); i++)
		{
		Printf(_L("Personality %d\r\n"), ids[i]);
		}

	if (iArguments.IsPresent(&iPersonality))
		{
		TRequestStatus stat;
		iUsb.TryStart(iPersonality, stat);
		User::WaitForRequest(stat);
		LeaveIfErr(stat.Int(), _L("Couldn't start USB personality %d"), iPersonality);
		Printf(_L("USB started ok.\r\n"));
		}
	}
