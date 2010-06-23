// vtc_cursor_tracker.h
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

#ifndef VTC_CURSOR_TRACKER_H
#define VTC_CURSOR_TRACKER_H

#include <e32std.h>


/**
 * This class is responsible for tracking the cursor position within the console
 * window based on the data that is written to it. The cursor position is tracked
 * in this way, rather than explicitly enquiring the cursor position from the
 * VT100 terminal (using "\x1b[6n") because is seems different VT100 emulators
 * respond to this escape sequence in different and unpredictable ways.
 */
class TCursorTracker
	{
public:
	TCursorTracker(TSize aConsoleSize);
	void Write(const TDesC& aDes);
	void Write(const TDesC8& aDes);
	void SetCursorPosAbs(const TPoint& aPoint);
	void SetCursorPosRel(const TPoint& aPoint);
	void Reset();
	TPoint CursorPos() const;
	TSize ConsoleSize() const;
private:
	void CursorLeft();
	void CursorRight();
	void LineFeed();
	void CarriageReturn();
	void WriteChar(TChar aChar);
private:
	const TSize iConsoleSize;
	TPoint iCursorPos;
	};


#endif // VTC_CURSOR_TRACKER_H
