# guicons_makefont.mk
# 
# Copyright (c) 2010 Accenture. All rights reserved.
# This component and the accompanying materials are made available
# under the terms of the "Eclipse Public License v1.0"
# which accompanies this distribution, and is available
# at the URL "http://www.eclipse.org/legal/epl-v10.html".
# 
# Initial Contributors:
# Accenture - Initial contribution
#
GENERATED_SOURCE_DIR = $(EPOCROOT)epoc32\build\fshell/guicons\generated
GENERATED_SOURCE = $(GENERATED_SOURCE_DIR)\defaultfontdata.inl

MAKMAKE : $(GENERATED_SOURCE)

$(GENERATED_SOURCE) : ..\data\font_6x10.PNG
	perl -S emkdir.pl $(GENERATED_SOURCE_DIR)
	echo perl .\bin2lit.pl KDefaultFontImageData ^< ..\data\font_6x10.PNG ^> $(GENERATED_SOURCE)
	perl .\bin2lit.pl KDefaultFontImageData < ..\data\font_6x10.PNG > $(GENERATED_SOURCE)

CLEAN :
	perl -S ermdir.pl $(GENERATED_SOURCE_DIR)

BLD FREEZE RESOURCE LIB CLEANLIB RESOURCE SAVESPACE RELEASABLES FINAL : 
