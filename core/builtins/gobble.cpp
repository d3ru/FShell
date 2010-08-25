// gobble.cpp
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

#include "gobble.h"


CCommandBase* CCmdGobble::NewLC()
	{
	CCmdGobble* self = new(ELeave) CCmdGobble();
	CleanupStack::PushL(self);
	self->BaseConstructL();
	return self;
	}

CCmdGobble::~CCmdGobble()
	{
	}

CCmdGobble::CCmdGobble()
	: iBlockSize(512)
	{
	}

const TDesC& CCmdGobble::Name() const
	{
	_LIT(KName, "gobble");
	return KName;
	}

void CCmdGobble::DoRunL()
	{
	if (iAmount < iBlockSize)
		{
		LeaveIfErr(KErrArgument, _L("The amount to consume must be less than the block size (%d)"), iBlockSize);
		}
	if (iAmount & 0x80000000)
		{
		LeaveIfErr(KErrArgument, _L("The amount to consume is too large (maximum is %d)"), KMaxTInt);
		}
	if (iBlockSize & 0x80000000)
		{
		LeaveIfErr(KErrArgument, _L("The block size is too large (maximum is %d)"), KMaxTInt);
		}
	RFs& fs = FsL();
	fs.MkDirAll(iFileName);
	RFile file;
	TInt err = file.Open(fs, iFileName, EFileWrite);
	if (err == KErrNotFound)
		{
		err = file.Create(fs, iFileName, EFileWrite);
		}
	User::LeaveIfError(err);
	CleanupClosePushL(file);
	TInt pos = 0;
	User::LeaveIfError(file.Seek(ESeekEnd, pos));

	HBufC8* buf = HBufC8::NewLC(iBlockSize);
	TPtr8 ptr(buf->Des());
	ptr.Fill(TChar('x'), iBlockSize);
	
	TInt toWrite = static_cast<TInt>(iAmount);
	do
		{
		TInt writeSize;
		if (toWrite > static_cast<TInt>(iBlockSize))
			{
			writeSize = static_cast<TInt>(iBlockSize);
			}
		else
			{
			writeSize = toWrite;
			}
		ptr.SetLength(writeSize);
		err = file.Write(ptr);
		if (err == KErrNone)
			{
			if (iVerbose)
				{
				Printf(_L("\rWrote %d"), iAmount - toWrite);
				}
			toWrite -= writeSize;
			}
		}
		while ((err == KErrNone) && (toWrite > 0));
		if (iVerbose)
			{
			Printf(_L("\rWrote %d"), iAmount - toWrite);
			}
		
	CleanupStack::PopAndDestroy(2, &file);
	}

void CCmdGobble::OptionsL(RCommandOptionList& aOptions)
	{
	_LIT(KCmdOptVerbose, "verbose");
	aOptions.AppendBoolL(iVerbose, KCmdOptVerbose);
	}

void CCmdGobble::ArgumentsL(RCommandArgumentList& aArguments)
	{
	_LIT(KArgFileName, "file_name");
	aArguments.AppendFileNameL(iFileName, KArgFileName);

	_LIT(KArgAmount, "amount");
	aArguments.AppendUintL(iAmount, KArgAmount);

	_LIT(KArgBlockSize, "block_size");
	aArguments.AppendUintL(iBlockSize, KArgBlockSize);
	}


#ifdef EXE_BUILD
EXE_BOILER_PLATE(CCmdGobble)
#endif

