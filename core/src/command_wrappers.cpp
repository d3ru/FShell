// command_wrappers.cpp
// 
// Copyright (c) 2006 - 2010 Accenture. All rights reserved.
// This component and the accompanying materials are made available
// under the terms of the "Eclipse Public License v1.0"
// which accompanies this distribution, and is available
// at the URL "http://www.eclipse.org/legal/epl-v10.html".
// 
// Initial Contributors:
// Accenture - Initial contribution
//

#include "command_wrappers.h"


//
// Constants.
//

const TInt KMaxHeapSize = KMinHeapSize * 1024;


//
// CCommandWrapperBase.
//

CCommandWrapperBase::CCommandWrapperBase()
	{
	}

CCommandWrapperBase::~CCommandWrapperBase()
	{
	iStdin.Close();
	iStdout.Close();
	iStderr.Close();
	delete iName;
	}

void CCommandWrapperBase::BaseConstructL(const TDesC& aName)
	{
	iName = aName.AllocL();
	}

RIoReadHandle& CCommandWrapperBase::CmndStdin()
	{
	return iStdin;
	}

RIoWriteHandle& CCommandWrapperBase::CmndStdout()
	{
	return iStdout;
	}

RIoWriteHandle& CCommandWrapperBase::CmndStderr()
	{
	return iStderr;
	}

const TDesC& CCommandWrapperBase::CmndName() const
	{
	return *iName;
	}

TInt CCommandWrapperBase::CmndReattachStdin(RIoEndPoint& aStdinEndPoint)
	{
	TInt isForeground = iStdin.IsForeground();
	if (isForeground < 0)
		{
		return isForeground;
		}
	return aStdinEndPoint.Attach(iStdin, isForeground ? RIoEndPoint::EForeground : RIoEndPoint::EBackground);
	}

TInt CCommandWrapperBase::CmndReattachStdout(RIoEndPoint& aStdoutEndPoint)
	{
	return aStdoutEndPoint.Attach(iStdout);
	}

TInt CCommandWrapperBase::CmndReattachStderr(RIoEndPoint& aStderrEndPoint)
	{
	return aStderrEndPoint.Attach(iStderr);
	}

TBool CCommandWrapperBase::CmndIsDisownable() const
	{
	return EFalse;
	}

void CCommandWrapperBase::CmndDisown()
	{
	ASSERT(EFalse);
	}

void CCommandWrapperBase::CmndRelease()
	{
	delete this;
	}


//
// CThreadCommand.
//

CThreadCommand* CThreadCommand::NewL(const TDesC& aName, TCommandConstructor aCommandConstructor, TUint aFlags)
	{
	CThreadCommand* self = new(ELeave) CThreadCommand(aCommandConstructor, aFlags);
	CleanupStack::PushL(self);
	self->ConstructL(aName);
	CleanupStack::Pop(self);
	return self;
	}

CThreadCommand::~CThreadCommand()
	{
	delete iWatcher;
	delete iArgs;
	iThread.Close();
	}

CThreadCommand::CThreadCommand(TCommandConstructor aCommandConstructor, TUint aFlags)
	: iFlags(aFlags), iCommandConstructor(aCommandConstructor)
	{
	iThread.SetHandle(0); // By default RThread refers to the current thread. This results in fshell's thread exiting if this object gets killed before it has managed to open a real thread handle.
	if (iFlags & EUpdateEnvironment) iFlags |= ESharedHeap; // Update environment implies a shared heap, ever since we did away with the explict SwitchAllocator
	}

void CThreadCommand::ConstructL(const TDesC& aName)
	{
	BaseConstructL(aName);
	iWatcher = CThreadWatcher::NewL();
	}

void CommandThreadStartL(CThreadCommand::TArgs& aArgs)
	{
	if (aArgs.iFlags & CThreadCommand::ESharedHeap)
		{
		// If we're sharing the main fshell heap, we have to play by the rules and not crash
		User::SetCritical(User::EProcessCritical);
		}

	CActiveScheduler* scheduler = new(ELeave) CActiveScheduler;
	CleanupStack::PushL(scheduler);
	CActiveScheduler::Install(scheduler);

	HBufC* commandLine = aArgs.iCommandLine.AllocLC();

	IoUtils::CEnvironment* env;
	if (aArgs.iFlags & CThreadCommand::EUpdateEnvironment)
		{
		env = aArgs.iEnv.CreateSharedEnvironmentL();
		}
	else
		{
		// A straight-forward copy
		env = IoUtils::CEnvironment::NewL(aArgs.iEnv);
		}
	CleanupStack::PushL(env);

	CCommandBase* command = (*aArgs.iCommandConstructor)();
	RThread parentThread;
	User::LeaveIfError(parentThread.Open(aArgs.iParentThreadId));
	parentThread.RequestComplete(aArgs.iParentStatus, KErrNone);
	parentThread.Close();

	command->RunCommandL(commandLine, env);
	CleanupStack::PopAndDestroy(4, scheduler); // env, command, commandline, scheduler
	}

TInt CommandThreadStart(TAny* aPtr)
	{
	CThreadCommand::TArgs args = *(CThreadCommand::TArgs*)aPtr;
	TBool sharedHeap = (args.iFlags & CThreadCommand::ESharedHeap);
	if (!sharedHeap)
		{
		__UHEAP_MARK;
		}
	TInt err = KErrNoMemory;
	CTrapCleanup* cleanup = CTrapCleanup::New();
	if (cleanup)
		{
		TRAP(err, CommandThreadStartL(args));
		delete cleanup;
		}
	if (!sharedHeap)
		{
		__UHEAP_MARKEND;
		}
	return err;
	}

void SetHandleOwnersL(TThreadId aThreadId, RIoReadHandle& aStdin, RIoWriteHandle& aStdout, RIoWriteHandle& aStderr)
	{
	User::LeaveIfError(aStdin.SetOwner(aThreadId));
	User::LeaveIfError(aStdout.SetOwner(aThreadId));
	User::LeaveIfError(aStderr.SetOwner(aThreadId));
	};

TInt CThreadCommand::CmndRun(const TDesC& aCommandLine, IoUtils::CEnvironment& aEnv, MCommandObserver& aObserver, RIoSession&)
	{
	ASSERT(iObserver == NULL);

	TRequestStatus status(KRequestPending);
	iArgs = new TArgs(iFlags, aEnv, iCommandConstructor, aCommandLine, status);
	if (iArgs == NULL)
		{
		return KErrNoMemory;
		}

	TInt i = 0;
	TName threadName;
	TInt err = KErrNone;
	do
		{
		const TDesC& name = CmndName();
		threadName.Format(_L("%S_%02d"), &name, i++);
		if (iFlags & ESharedHeap)
			{
			err = iThread.Create(threadName, CommandThreadStart, KDefaultStackSize, NULL, iArgs);
			}
		else
			{
			err = iThread.Create(threadName, CommandThreadStart, KDefaultStackSize, KMinHeapSize, KMaxHeapSize, iArgs);
			}
		}
		while (err == KErrAlreadyExists);

	if (err)
		{
		return err;
		}

	err = iWatcher->Logon(*this, iThread, aObserver);
	if (err)
		{
		iThread.Kill(0);
		iThread.Close();
		return err;
		}

	TThreadId threadId = iThread.Id();
	TRAP(err, SetHandleOwnersL(threadId, CmndStdin(), CmndStdout(), CmndStderr()));
	if (err)
		{
		iThread.Kill(0);
		iThread.Close();
		return err;
		}

	iThread.Resume();
	User::WaitForRequest(status, iWatcher->iStatus);
	if (status == KRequestPending)
		{
		iThread.Close();
		return iWatcher->iStatus.Int();
		}

	iWatcher->SetActive();
	iObserver = &aObserver;
	return KErrNone;
	}

void CThreadCommand::CmndForeground()
	{
	iThread.SetPriority(EPriorityAbsoluteForeground);
	}

void CThreadCommand::CmndBackground()
	{
	iThread.SetPriority(EPriorityAbsoluteBackground);
	}

void CThreadCommand::CmndKill()
	{
	if (iThread.Handle())
		{
		iThread.Kill(KErrAbort);
		}
	}

TInt CThreadCommand::CmndSuspend()
	{
	iThread.Suspend();
	return KErrNone;
	}

TInt CThreadCommand::CmndResume()
	{
	iThread.Resume();
	return KErrNone;
	}

TExitType CThreadCommand::CmndExitType() const
	{
	return iThread.ExitType();
	}


TExitCategoryName CThreadCommand::CmndExitCategory() const
	{
	return iThread.ExitCategory();
	}


//
// CThreadCommand::TArgs.
//

CThreadCommand::TArgs::TArgs(TUint aFlags, IoUtils::CEnvironment& aEnv, TCommandConstructor aCommandConstructor, const TDesC& aCommandLine, TRequestStatus& aParentStatus)
	: iFlags(aFlags), iEnv(aEnv), iCommandConstructor(aCommandConstructor), iCommandLine(aCommandLine), iParentStatus(&aParentStatus), iParentThreadId(RThread().Id())
	{
	}


//
// CThreadCommand::CThreadWatcher.
//

CThreadCommand::CThreadWatcher* CThreadCommand::CThreadWatcher::NewL()
	{
	return new(ELeave) CThreadWatcher();
	}

CThreadCommand::CThreadWatcher::~CThreadWatcher()
	{
	Cancel();
	}

CThreadCommand::CThreadWatcher::CThreadWatcher()
	: CActive(CActive::EPriorityStandard)
	{
	CActiveScheduler::Add(this);
	}

TInt CThreadCommand::CThreadWatcher::Logon(CThreadCommand& aCommand, RThread& aThread, MCommandObserver& aObserver)
	{
	TInt ret = KErrNone;
	aThread.Logon(iStatus);
	if (iStatus != KRequestPending)
		{
		User::WaitForRequest(iStatus);
		ret = iStatus.Int();
		}
	else
		{
		iCommand = &aCommand;
		iThread = &aThread;
		iObserver = &aObserver;
		}
	return ret;
	}

void CThreadCommand::CThreadWatcher::SetActive()
	{
	CActive::SetActive();
	}

void CThreadCommand::CThreadWatcher::RunL()
	{
	iObserver->HandleCommandComplete(*iCommand, iStatus.Int());
	}

void CThreadCommand::CThreadWatcher::DoCancel()
	{
	if (iThread)
		{
		iThread->LogonCancel(iStatus);
		}
	}


//
// CProcessCommand.
//

CProcessCommand* CProcessCommand::NewL(const TDesC& aExeName)
	{
	CProcessCommand* self = new(ELeave) CProcessCommand();
	CleanupStack::PushL(self);
	self->ConstructL(aExeName);
	CleanupStack::Pop(self);
	return self;
	}

CProcessCommand* CProcessCommand::NewL(const TDesC& aExeName, RProcess& aProcess)
	{
	CProcessCommand* self = new(ELeave) CProcessCommand();
	CleanupStack::PushL(self);
	self->ConstructL(aExeName, &aProcess);
	CleanupStack::Pop(self);
	return self;
	}


CProcessCommand::~CProcessCommand()
	{
	delete iWatcher;
	iProcess.Close();
	}

CProcessCommand::CProcessCommand()
	{
	iProcess.SetHandle(KNullHandle); // By default RProcess refers to the current process (KCurrentProcessHandle). This results in fshell's process exiting if this object gets killed before it has managed to open a real process handle.
	}

void CProcessCommand::ConstructL(const TDesC& aExeName, RProcess* aProcess)
	{
	BaseConstructL(aExeName);
	iWatcher = CProcessWatcher::NewL();
	if (aProcess)
		{
		// Don't take ownership of the aProcess handle until after everything that can leave has been run
		iProcess.SetHandle(aProcess->Handle());
		}
	}

void CProcessCommand::CreateProcessL(const TDesC& aCommandLine, IoUtils::CEnvironment&)
	{
	if (iProcess.Handle() == KNullHandle)
		{
		// Don't create new proc if we were passed one in initially
		User::LeaveIfError(iProcess.Create(CmndName(), aCommandLine));
		}
	}

void CProcessCommand::CmndRunL(const TDesC& aCommandLine, IoUtils::CEnvironment& aEnv, MCommandObserver& aObserver)
	{
	CreateProcessL(aCommandLine, aEnv);

	HBufC8* envBuf = aEnv.ExternalizeLC();
	User::LeaveIfError(iProcess.SetParameter(IoUtils::KEnvironmentProcessSlot, *envBuf));
	CleanupStack::PopAndDestroy(envBuf);

	TFullName fullName(iProcess.Name());
	_LIT(KThreadName,"::Main");
	fullName.Append(KThreadName);
	RThread thread;
	User::LeaveIfError(thread.Open(fullName));
	TThreadId threadId(thread.Id());
	thread.Close();
	User::LeaveIfError(CmndStdin().SetOwner(threadId));
	User::LeaveIfError(CmndStdout().SetOwner(threadId));
	User::LeaveIfError(CmndStderr().SetOwner(threadId));
	TInt err = iWatcher->Logon(*this, iProcess, aObserver);
	if (err)
		{
		iProcess.Kill(0);
		}
	User::LeaveIfError(err);
	iObserver = &aObserver;
	iProcess.Resume();
	}

TInt CProcessCommand::CmndRun(const TDesC& aCommandLine, IoUtils::CEnvironment& aEnv, MCommandObserver& aObserver, RIoSession&)
	{
	ASSERT(iObserver == NULL);
	TRAPD(err, CmndRunL(aCommandLine, aEnv, aObserver));
	return err;
	}

void CProcessCommand::CmndForeground()
	{
	iProcess.SetPriority(EPriorityForeground);
	}

void CProcessCommand::CmndBackground()
	{
	iProcess.SetPriority(EPriorityBackground);
	}

void CProcessCommand::CmndKill()
	{
	if (iProcess.Handle())
		{
		iProcess.Kill(KErrAbort);
		}
	}

TInt CProcessCommand::CmndSuspend()
	{
#ifdef EKA2
	// Can't currently support suspend in EKA2 - KERN-EXEC 46 will result.
	return KErrNotSupported;
#else
	TName processName(iProcess.Name());
	processName.Append(_L("::*"));
	TFullName threadName;
	TFindThread threadFinder(processName);
	while (threadFinder.Next(threadName) == KErrNone)
		{
		RThread thread;
		if (thread.Open(threadName) == KErrNone)
			{
			thread.Suspend();
			thread.Close();
			}
		}
	return KErrNone;
#endif
	}

TInt CProcessCommand::CmndResume()
	{
#ifdef EKA2
	// Can't currently support resume in EKA2 - KERN-EXEC 46 will result.
	return KErrNotSupported;
#else
	TName processName(iProcess.Name());
	processName.Append(_L("::*"));
	TFullName threadName;
	TFindThread threadFinder(processName);
	while (threadFinder.Next(threadName) == KErrNone)
		{
		RThread thread;
		if (thread.Open(threadName) == KErrNone)
			{
			thread.Resume();
			thread.Close();
			}
		}
	return KErrNone;
#endif
	}

TExitType CProcessCommand::CmndExitType() const
	{
	return iProcess.ExitType();
	}

TExitCategoryName CProcessCommand::CmndExitCategory() const
	{
	return iProcess.ExitCategory();
	}

TBool CProcessCommand::CmndIsDisownable() const
	{
	return ETrue;
	}

void CProcessCommand::CmndDisown()
	{
	delete iWatcher;
	iWatcher = NULL;
	iProcess.Close();
	}


//
// CProcessCommand::CProcessWatcher.
//

CProcessCommand::CProcessWatcher* CProcessCommand::CProcessWatcher::NewL()
	{
	return new(ELeave) CProcessWatcher();
	}

CProcessCommand::CProcessWatcher::~CProcessWatcher()
	{
	Cancel();
	}

TInt CProcessCommand::CProcessWatcher::Logon(CProcessCommand& aCommand, RProcess& aProcess, MCommandObserver& aObserver)
	{
	TInt ret = KErrNone;
	aProcess.Logon(iStatus);
	if (iStatus != KRequestPending)
		{
		User::WaitForRequest(iStatus);
		ret = iStatus.Int();
		}
	else
		{
		iCommand = &aCommand;
		iProcess = &aProcess;
		iObserver = &aObserver;
		SetActive();
		}
	return ret;
	}

CProcessCommand::CProcessWatcher::CProcessWatcher()
	: CActive(CActive::EPriorityStandard)
	{
	CActiveScheduler::Add(this);
	}

void CProcessCommand::CProcessWatcher::RunL()
	{
	iObserver->HandleCommandComplete(*iCommand, iStatus.Int());
	}

void CProcessCommand::CProcessWatcher::DoCancel()
	{
	if (iProcess)
		{
		iProcess->LogonCancel(iStatus);
		}
	}

//
// CPipsCommand.
//

CPipsCommand* CPipsCommand::NewL(const TDesC& aExeName)
	{
	CPipsCommand* self = new (ELeave) CPipsCommand();
	CleanupStack::PushL(self);
	self->ConstructL(aExeName);
	CleanupStack::Pop(self);
	return self;
	}

CPipsCommand::CPipsCommand()
	{
	}

CPipsCommand::~CPipsCommand()
	{
	}

TInt CPipsCommand::CmndRun(const TDesC& aCommandLine, IoUtils::CEnvironment& aEnv, MCommandObserver& aObserver, RIoSession& aIoSession)
	{
	TInt err = CProcessCommand::CmndRun(aCommandLine, aEnv, aObserver, aIoSession);
	if ((err == KErrNone) && iUsingPipsRun)
		{
		TRequestStatus status;
		iProcess.Rendezvous(status);
		User::WaitForRequest(status);
		err = status.Int();
		if (err > 0)
			{
			iPipsRunChildProcessId = err;
			err = KErrNone;
			}
		}
	return err;
	}

void CPipsCommand::CmndKill()
	{
	CProcessCommand::CmndKill();

	if (iUsingPipsRun)
		{
		RProcess pipsRunChildProcess;
		if (pipsRunChildProcess.Open(iPipsRunChildProcessId) == KErrNone)
			{
			pipsRunChildProcess.Kill(KErrAbort);
			pipsRunChildProcess.Close();
			}
		}
	}

void CPipsCommand::CreateProcessL(const TDesC& aCommandLine, IoUtils::CEnvironment&)
	{
	_LIT(KPipsRunExe, "pipsrun");
	TInt err = iProcess.Create(KPipsRunExe, KNullDesC);
	if (err == KErrNotFound)
		{
		// Looks like pipsrun.exe isn't present - just load the PIPS exe directly.
		User::LeaveIfError(iProcess.Create(CmndName(), aCommandLine));
		}
	else
		{
		User::LeaveIfError(err);
		User::LeaveIfError(iProcess.SetParameter(IoUtils::KPipsCommandNameProcessSlot, CmndName()));
		User::LeaveIfError(iProcess.SetParameter(IoUtils::KPipsCommandLineProcessSlot, aCommandLine));
		iUsingPipsRun = ETrue;
		}
	}


//
// CAliasCommand.
//

CAliasCommand* CAliasCommand::NewL(MCommand& aAliasedCommand, const TDesC* aAdditionalArguments, const TDesC* aReplacementArguments)
	{
	CAliasCommand* self = new(ELeave) CAliasCommand(aAliasedCommand);
	CleanupStack::PushL(self);
	self->ConstructL(aAdditionalArguments, aReplacementArguments);
	CleanupStack::Pop(self);
	return self;
	}

CAliasCommand::~CAliasCommand()
	{
	delete iAdditionalArguments;
	delete iReplacementArguments;
	iAliasedCommand.CmndRelease();
	}

CAliasCommand::CAliasCommand(MCommand& aAliasedCommand)
	: iAliasedCommand(aAliasedCommand)
	{
	}

void CAliasCommand::ConstructL(const TDesC* aAdditionalArguments, const TDesC* aReplacementArguments)
	{
	BaseConstructL(iAliasedCommand.CmndName());
	if (aAdditionalArguments)
		{
		iAdditionalArguments = aAdditionalArguments->AllocL();
		}
	if (aReplacementArguments)
		{
		iReplacementArguments = aReplacementArguments->AllocL();
		}
	}

RIoReadHandle& CAliasCommand::CmndStdin()
	{
	return iAliasedCommand.CmndStdin();
	}

RIoWriteHandle& CAliasCommand::CmndStdout()
	{
	return iAliasedCommand.CmndStdout();
	}

RIoWriteHandle& CAliasCommand::CmndStderr()
	{
	return iAliasedCommand.CmndStderr();
	}

TInt CAliasCommand::CmndRun(const TDesC& aCommandLine, IoUtils::CEnvironment& aEnv, MCommandObserver& aObserver, RIoSession& aIoSession)
	{
	if (iAdditionalArguments && !iReplacementArguments)
		{
		iAdditionalArguments = iAdditionalArguments->ReAlloc(iAdditionalArguments->Length() + aCommandLine.Length() + 1);
		if (iAdditionalArguments == NULL)
			{
			return KErrNoMemory;
			}
		_LIT(KSpace, " ");
		iAdditionalArguments->Des().Append(KSpace);
		iAdditionalArguments->Des().Append(aCommandLine);
		}
	iCommandObserver = &aObserver;
	const TDesC* args = &aCommandLine;
	if (iReplacementArguments)
		{
		args = iReplacementArguments;
		}
	else if (iAdditionalArguments)
		{
		args = iAdditionalArguments;
		}
	return iAliasedCommand.CmndRun(*args, aEnv, *this, aIoSession);
	}

void CAliasCommand::CmndForeground()
	{
	iAliasedCommand.CmndForeground();
	}

void CAliasCommand::CmndBackground()
	{
	iAliasedCommand.CmndBackground();
	}

void CAliasCommand::CmndKill()
	{
	iAliasedCommand.CmndKill();
	}

TInt CAliasCommand::CmndSuspend()
	{
	return iAliasedCommand.CmndSuspend();
	}

TInt CAliasCommand::CmndResume()
	{
	return iAliasedCommand.CmndResume();
	}

TExitType CAliasCommand::CmndExitType() const
	{
	return iAliasedCommand.CmndExitType();
	}

TExitCategoryName CAliasCommand::CmndExitCategory() const
	{
	return iAliasedCommand.CmndExitCategory();
	}

void CAliasCommand::HandleCommandComplete(MCommand&, TInt aError)
	{
	iCommandObserver->HandleCommandComplete(*this, aError);
	}
