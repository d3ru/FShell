// btrace_appresponse.cpp
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

#include "btrace_parser.h"
#include <fshell/common.mmh>


//
// CBtraceAppResponse
//

EXPORT_C CBtraceAppResponse* CBtraceAppResponse::NewL(CBtraceReader& aReader, CBtraceContext& aContext)
	{
	CBtraceAppResponse* self = new (ELeave) CBtraceAppResponse(aReader, aContext);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}

CBtraceAppResponse::CBtraceAppResponse(CBtraceReader& aReader, CBtraceContext& aContext)
	: iReader(aReader), iContext(aContext)
	{
	}

EXPORT_C CBtraceAppResponse::~CBtraceAppResponse()
	{
	iReader.RemoveObserver(KAmTraceCategory, *this);
	iNotifs.Close();
	}

void CBtraceAppResponse::ConstructL()
	{
	iReader.AddObserverL(KAmTraceCategory, *this);
	}

void CBtraceAppResponse::HandleBtraceFrameL(const TBtraceFrame& aFrame)
	{
	if (aFrame.iCategory != KAmTraceCategory) return;

	switch (aFrame.iSubCategory)
		{
		case EAmTraceSubCategoryEvCapture:
			{
			const TUint32* data = reinterpret_cast<const TUint32*>(aFrame.iData.Ptr());
			const TAmTraceEventEvCapture event = static_cast<TAmTraceEventEvCapture>(*data++);

			switch (event)
				{
				case EAmTraceEventEvCaptureAppResponse:
					{
					const TInt32 wgId = *data++;
					SeenAppResponseL(aFrame.iTickCount, wgId);
					}
				break;
				
				default:
					// ignore the event
				break;
				};
			}
		break;

		default:
			// ignore anything we don't know about.
		break;
		};
	}

void CBtraceAppResponse::SeenAppResponseL(const TBtraceTickCount& aTickCount, TInt aWindowGroupId)
	{
	const TBtraceWindowGroupId* btraceWgId = iContext.FindWindowGroup(aWindowGroupId);
	if (btraceWgId)
		{
		TInt ii = iNotifs.Count();
		while (--ii >= 0)
			{
			TAppResponseNotif& nt = iNotifs[ii];
			if (iContext.WindowGroupName(*btraceWgId).MatchF(nt.iWindowGroupNamePattern) != KErrNotFound)
				{
				MBtraceAppResponseObserver& observer = nt.iObserver;
				if (nt.iPersistence == ENotificationOneShot)
					{
					iNotifs.Remove(ii);
					}
				observer.HandleAppResponseSeenL(aTickCount);
				}
			}
		}
	}

EXPORT_C void CBtraceAppResponse::NotifyAppResponseL(MBtraceAppResponseObserver& aObserver, const TDesC& aWindowGroupNamePattern)
	{
	NotifyAppResponseL(aObserver, aWindowGroupNamePattern, ENotificationOneShot);
	}

EXPORT_C void CBtraceAppResponse::NotifyAppResponseL(MBtraceAppResponseObserver& aObserver, const TDesC& aWindowGroupNamePattern, TBtraceNotificationPersistence aPersistence)
	{
	TAppResponseNotif notify(aObserver, aWindowGroupNamePattern, aPersistence);
	User::LeaveIfError(iNotifs.Append(notify));
	}

EXPORT_C void CBtraceAppResponse::CancelNotifyAppResponse(MBtraceAppResponseObserver& aObserver)
	{
	for (TInt i = (iNotifs.Count() - 1); i >= 0; --i)
		{
		if (&iNotifs[i].iObserver == &aObserver)
			{
			iNotifs.Remove(i);
			}
		}
	}


//
// CBtraceAppResponse::TAppResponseNotif
//

CBtraceAppResponse::TAppResponseNotif::TAppResponseNotif(MBtraceAppResponseObserver& aObserver, const TDesC& aWindowGroupNamePattern, TBtraceNotificationPersistence aPersistence)
	: iObserver(aObserver), iWindowGroupNamePattern(aWindowGroupNamePattern), iPersistence(aPersistence)
	{
	}
