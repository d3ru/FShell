// ltkhal.cpp
// 
// Copyright (c) 2010 - 2012 Accenture. All rights reserved.
// This component and the accompanying materials are made available
// under the terms of the "Eclipse Public License v1.0"
// which accompanies this distribution, and is available
// at the URL "http://www.eclipse.org/legal/epl-v10.html".
// 
// Initial Contributors:
// Accenture - Initial contribution
//
#include <fshell/ltkutils.h>
#include <fshell/ltkhal.h>
#include <fshell/descriptorutils.h>
#include <fshell/common.mmh>
#include <HAL.h>
#include <f32file.h>
#include <u32std.h> // For EHalGroupKernel

LtkUtils::CHalAttribute::CHalAttribute(TInt aAttribute, TInt aDeviceNumber, TInt aValue, TInt aProperties)
	: iAttribute(aAttribute), iDeviceNumber(aDeviceNumber), iValue(aValue), iProperties(aProperties), iDescription(NULL)
	{
	}

EXPORT_C LtkUtils::CHalAttribute::~CHalAttribute()
	{
	delete iDescription;
	}

const LtkUtils::SLitC KHalNames[] =
	{
	DESC("EManufacturer"),
	DESC("EManufacturerHardwareRev"),
	DESC("EManufacturerSoftwareRev"),
	DESC("EManufacturerSoftwareBuild"),
	DESC("EModel"),
	DESC("EMachineUid"),
	DESC("EDeviceFamily"),
	DESC("EDeviceFamilyRev"),
	DESC("ECPU"),
	DESC("ECPUArch"),
	DESC("ECPUABI"),
	DESC("ECPUSpeed"),
	DESC("ESystemStartupReason"),
	DESC("ESystemException"),
	DESC("ESystemTickPeriod"),
	DESC("EMemoryRAM"),
	DESC("EMemoryRAMFree"),
	DESC("EMemoryROM"),
	DESC("EMemoryPageSize"),
	DESC("EPowerGood"),
	DESC("EPowerBatteryStatus"),
	DESC("EPowerBackup"),
	DESC("EPowerBackupStatus"),
	DESC("EPowerExternal"),
	DESC("EKeyboard"),
	DESC("EKeyboardDeviceKeys"),
	DESC("EKeyboardAppKeys"),
	DESC("EKeyboardClick"),
	DESC("EKeyboardClickState"),
	DESC("EKeyboardClickVolume"),
	DESC("EKeyboardClickVolumeMax"),
	DESC("EDisplayXPixels"),
	DESC("EDisplayYPixels"),
	DESC("EDisplayXTwips"),
	DESC("EDisplayYTwips"),
	DESC("EDisplayColors"),
	DESC("EDisplayState"),
	DESC("EDisplayContrast"),
	DESC("EDisplayContrastMax"),
	DESC("EBacklight"),
	DESC("EBacklightState"),
	DESC("EPen"),
	DESC("EPenX"),
	DESC("EPenY"),
	DESC("EPenDisplayOn"),
	DESC("EPenClick"),
	DESC("EPenClickState"),
	DESC("EPenClickVolume"),
	DESC("EPenClickVolumeMax"),
	DESC("EMouse"),
	DESC("EMouseX"),
	DESC("EMouseY"),
	DESC("EMouseState"),
	DESC("EMouseSpeed"),
	DESC("EMouseAcceleration"),
	DESC("EMouseButtons"),
	DESC("EMouseButtonState"),
	DESC("ECaseState"),
	DESC("ECaseSwitch"),
	DESC("ECaseSwitchDisplayOn"),
	DESC("ECaseSwitchDisplayOff"),
	DESC("ELEDs"),
	DESC("ELEDmask"),
	DESC("EIntegratedPhone"),
	DESC("EDisplayBrightness"),
	DESC("EDisplayBrightnessMax"),
	DESC("EKeyboardBacklightState"),
	DESC("EAccessoryPower"),
	DESC("ELanguageIndex"),
	DESC("EKeyboardIndex"),
	DESC("EMaxRAMDriveSize"),
	DESC("EKeyboardState"),
	DESC("ESystemDrive"),
	DESC("EPenState"),
	DESC("EDisplayIsMono"),
	DESC("EDisplayIsPalettized"),
	DESC("EDisplayBitsPerPixel"),
	DESC("EDisplayNumModes"),
	DESC("EDisplayMemoryAddress"),
	DESC("EDisplayOffsetToFirstPixel"),
	DESC("EDisplayOffsetBetweenLines"),
	DESC("EDisplayPaletteEntry"),
	DESC("EDisplayIsPixelOrderRGB"),
	DESC("EDisplayIsPixelOrderLandscape"),
	DESC("EDisplayMode"),
	DESC("ESwitches"),
	DESC("EDebugPort"),
	DESC("ELocaleLoaded"),
	DESC("EClipboardDrive"),
	DESC("ECustomRestart"),
	DESC("ECustomRestartReason"),
	DESC("EDisplayNumberOfScreens"),
	DESC("ENanoTickPeriod"),
	DESC("EFastCounterFrequency"),
	DESC("EFastCounterCountsUp"),
	DESC("EPointer3D"),
	DESC("EPointer3DZ/EPointer3DMaxProximity"),
	DESC("EPointer3DThetaSupported"),
	DESC("EPointer3DPhiSupported"),
	DESC("EPointer3DRotationSupported"),
	DESC("EPointer3DPressureSupported"),
	DESC("EHardwareFloatingPoint"),
	DESC("ETimeNonSecureOffset"),
	DESC("EPersistStartupModeKernel"),
	DESC("EMaximumCustomRestartReasons"),
	DESC("EMaximumRestartStartupModes"),
	DESC("ECustomResourceDrive"),
	DESC("EPointer3DProximityStep"), 
	DESC("EPointerMaxPointers"),
	DESC("EPointerNumberOfPointers"),
	DESC("EPointer3DMaxPressure"),
	DESC("EPointer3DPressureStep"),
	DESC("EPointer3DEnterHighPressureThreshold"),
	DESC("EPointer3DExitHighPressureThreshold"),
	DESC("EPointer3DEnterCloseProximityThreshold"),
	DESC("EPointer3DExitCloseProximityThreshold"),
	DESC("EDisplayMemoryHandle"),
	DESC("ESerialNumber"),
	DESC("ECpuProfilingDefaultInterruptBase"),
	DESC("ENumCpus"),
	DESC("EDigitiserOrientation"),
	};
const TInt KNumHalNames = sizeof(KHalNames) / sizeof(LtkUtils::SLitC);

const TDesC* Stringify(TInt aHalAttribute, TInt aValue)
	{
	_LIT(KManufacturerEricsson, "Ericsson");
	_LIT(KManufacturerMotorola, "Motorola");
	_LIT(KManufacturerNokia, "Nokia");
	_LIT(KManufacturerPanasonic, "Panasonic");
	_LIT(KManufacturerPsion, "Psion");
	_LIT(KManufacturerIntel, "Intel");
	_LIT(KManufacturerCogent, "Cogent");
	_LIT(KManufacturerCirrus, "Cirrus");
	_LIT(KManufacturerLinkup, "Linkup");
	_LIT(KCpuArm, "ARM");
	_LIT(KCpuMCore, "MCore");
	_LIT(KCpuX86, "x86");
	_LIT(KCpuAbiArm4, "ARM4");
	_LIT(KCpuAbiThumb, "THUMB");
	_LIT(KCpuAbiArmI, "ARMI");
	_LIT(KCpuAbiMCore, "MCore");
	_LIT(KCpuAbiMsvc, "MSVC");
	_LIT(KCpuAbiArm5T, "ARM5T");
	_LIT(KCpuAbiX86, "x86");
	_LIT(KStartupReasonCold, "Cold");
	_LIT(KStartupReasonWarm, "Warm");
	_LIT(KStartupReasonFault, "Fault");
	_LIT(KUp, "Up");
	_LIT(KDown, "Down");

	switch (aHalAttribute)
		{
		case HALData::EManufacturer:
			switch (aValue)
				{
				case HALData::EManufacturer_Ericsson:
					return &KManufacturerEricsson;
				case HALData::EManufacturer_Motorola:
					return &KManufacturerMotorola;
				case HALData::EManufacturer_Nokia:
					return &KManufacturerNokia;
				case HALData::EManufacturer_Panasonic:
					return &KManufacturerPanasonic;
				case HALData::EManufacturer_Psion:
					return &KManufacturerPsion;	
				case HALData::EManufacturer_Intel:
					return &KManufacturerIntel;
				case HALData::EManufacturer_Cogent:
					return &KManufacturerCogent;
				case HALData::EManufacturer_Cirrus:
					return &KManufacturerCirrus;
				case HALData::EManufacturer_Linkup:
					return &KManufacturerLinkup;
				default:
					break;
				}
			break;
		case HALData::ECPU:
			switch (aValue)
				{
				case HALData::ECPU_ARM:
					return &KCpuArm;
				case HALData::ECPU_MCORE:
					return &KCpuMCore;
				case HALData::ECPU_X86:
					return &KCpuX86;
				default:
					break;
				}
			break;
		case HALData::ECPUABI:
			switch (aValue)
				{
				case HALData::ECPUABI_ARM4:
					return &KCpuAbiArm4;
				case HALData::ECPUABI_ARMI:
					return &KCpuAbiMCore;
				case HALData::ECPUABI_THUMB:
					return &KCpuAbiThumb;
				case HALData::ECPUABI_MCORE:
					return &KCpuAbiArmI;
				case HALData::ECPUABI_MSVC:
					return &KCpuAbiMsvc;
				case HALData::ECPUABI_ARM5T:
					return &KCpuAbiArm5T;
				case HALData::ECPUABI_X86:
					return &KCpuAbiX86;
				default:
					break;
				}
			break;
		case HALData::ESystemStartupReason:
			switch (aValue)
				{
				case HALData::ESystemStartupReason_Cold:
					return &KStartupReasonCold;
				case HALData::ESystemStartupReason_Warm:
					return &KStartupReasonWarm;
				case HALData::ESystemStartupReason_Fault:
					return &KStartupReasonFault;
				default:
					break;
				}
			break;
		case HALData::EFastCounterCountsUp:
			if (aValue)
				{
				return &KUp;
				}
			else
				{
				return &KDown;
				}
		default:
			break;
		}

	return NULL;
	}




EXPORT_C void LtkUtils::GetHalInfoL(RPointerArray<CHalAttribute>& aAttributes)
	{
	aAttributes.ResetAndDestroy();
	HAL::SEntry* ents = NULL;
	TInt numEntries = 0;
	User::LeaveIfError(HAL::GetAll(numEntries, ents));
	CleanupDeletePushL(ents);

	for (TInt i = 0; i < numEntries; i++)
		{
		CHalAttribute* attrib = new(ELeave) CHalAttribute(i, 0, ents[i].iValue, ents[i].iProperties);
		CleanupStack::PushL(attrib);
		aAttributes.AppendL(attrib);
		CleanupStack::Pop(attrib);
		}
	CleanupStack::PopAndDestroy(ents);
	}

EXPORT_C LtkUtils::CHalAttribute* LtkUtils::GetHalInfoL(TInt aAttribute)
	{
	return GetHalInfoL(0, aAttribute);
	}

EXPORT_C LtkUtils::CHalAttribute* LtkUtils::GetHalInfoL(TInt aDeviceNumber, TInt aAttribute)
	{
	CHalAttribute* attrib = new(ELeave) CHalAttribute(aAttribute, aDeviceNumber, 0);
	TInt err = attrib->Update();

	if (err != KErrNone)
		{
		CleanupStack::PushL(attrib);
		RLtkBuf buf;
		buf.AppendFormatL(_L("HAL::Get() returned error %d"), err);
		attrib->SetDescription(buf.ToHBuf());
		CleanupStack::Pop(attrib);
		}
	return attrib;
	}

EXPORT_C LtkUtils::CHalAttribute* LtkUtils::GetHalInfoForValueL(TInt aDeviceNumber, TInt aAttribute, TInt aValue)
	{
	return new(ELeave) CHalAttribute(aAttribute, aDeviceNumber, aValue);
	}

EXPORT_C char LtkUtils::GetSystemDrive()
	{
#ifdef FSHELL_9_1_SUPPORT
	TInt ch = EDriveC;
	HAL::Get(HAL::ESystemDrive, ch);
	return 'a' + ch;
#else
	TChar systemDrive = 'c';
	RFs::DriveToChar(RFs::GetSystemDrive(), systemDrive);
	return (char)(TUint)systemDrive;
#endif
	}

EXPORT_C TInt LtkUtils::CHalAttribute::Attribute() const
	{
	return iAttribute;
	}

EXPORT_C const TDesC& LtkUtils::CHalAttribute::AttributeName() const
	{
	if (iAttribute < 0 || iAttribute >= KNumHalNames) return KNullDesC;
	else return KHalNames[iAttribute];
	}

EXPORT_C TInt LtkUtils::CHalAttribute::DeviceNumber() const
	{
	return iDeviceNumber;
	}

EXPORT_C TInt LtkUtils::CHalAttribute::Value() const
	{
	return iValue;
	}

EXPORT_C TInt LtkUtils::CHalAttribute::Update()
	{
	TInt val;
	TInt err = HAL::Get(iDeviceNumber, (HALData::TAttribute)iAttribute, val);

	if (err == KErrNone && val != iValue)
		{
		delete iDescription;
		iDescription = NULL;
		iValue = val;
		}
	return err;
	}

void LtkUtils::CHalAttribute::SetDescription(HBufC* aDescription)
	{
	delete iDescription;
	iDescription = aDescription;
	}

EXPORT_C const TDesC& LtkUtils::CHalAttribute::DescriptionL()
	{
	if (iDescription == NULL)
		{
		RLtkBuf buf;
		CleanupClosePushL(buf);

		if (iDeviceNumber != 0)
			{
			buf.AppendFormatL(_L("[Device %d] "), iDeviceNumber);
			}

		const TDesC* string = Stringify(iAttribute, iValue);
		if (string) buf.AppendL(*string);
		else
			{
			if ((iAttribute == HAL::ESystemDrive || iAttribute == HAL::EClipboardDrive) && iValue == -1)
				{
				// Emulator returns -1 for system drive...
				buf.AppendL(_L("Unknown"));
				}
			}
		if (buf.Length())
			{
			buf.AppendFormatL(_L(" (%d/0x%08x)"), iValue, iValue);
			}
		else
			{
			buf.AppendFormatL(_L("%d (0x%08x)"), iValue, iValue);
			}

		if (iProperties & HAL::EEntryDynamic) buf.AppendFormatL(_L(" [Dynamic]"));

		iDescription = buf.ToHBuf();
		CleanupStack::Pop(&buf);
		}
	return *iDescription;
	}

// Doesn't really belong in this file but there isn't anywhere better...

EXPORT_C TInt LtkUtils::LoadLogicalDevice(const TDesC& aFileName)
	{
	TInt err = User::LoadLogicalDevice(aFileName);
	if (err == KErrNotSupported && IsSmp())
		{
		// We're on SMP, need to try the SMP_ prefixed version of the driver
		// This is because the fshell SIS file cannot selectively install the correct driver, so it has to install both using the names driver.ldd and SMP_driver.ldd.
		_LIT(KPrefix, "SMP_");
		TBuf<32> name;
		name.Append(KPrefix);
		name.Append(aFileName);
		err = User::LoadLogicalDevice(name);
		}
	return err;
	}

EXPORT_C TBool LtkUtils::IsSmp()
	{
	enum THalFunctionsNotIn91
		{
		EKernelHalSmpSupported = 15,
		};
	TBool smpEnabled = (UserSvr::HalFunction(EHalGroupKernel, EKernelHalSmpSupported, 0, 0) == KErrNone);
	return smpEnabled;
	}
