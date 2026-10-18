#---------------------------------------------------------------------------------------------------------------------
# CHESS LEGENDS: ARCANE WARS - Game Boy Advance build (Butano + devkitARM).
#
# Windows: run .\build.ps1 (maps the project to an ASCII-only drive letter first; GNU make and gcc cannot handle
# the spaces / non-ASCII characters of the original folder name). Inside MSYS2: make -j8
#
# LIBBUTANO can be overridden from the environment: make LIBBUTANO=/path/to/butano/butano
#---------------------------------------------------------------------------------------------------------------------
TARGET      	:=  ChessLegends
BUILD       	:=  build
LIBBUTANO   	?=  /opt/butano/butano
PYTHON      	:=  python3
SOURCES     	:=  src core
INCLUDES    	:=  include core
DATA        	:=
GRAPHICS    	:=  graphics
AUDIO       	:=  audio
AUDIOBACKEND	:=  maxmod
AUDIOTOOL		:=
DMGAUDIO    	:=
DMGAUDIOBACKEND	:=  null
ROMTITLE    	:=  CHESSLEGENDS
ROMCODE     	:=  CLAW
# Text is drawn with sprites: every text sprite owns a tiles item, and a full redraw briefly holds the old and
# the new text, so the sprite tiles item pool is doubled (Butano default: 128).
USERFLAGS   	:=  -DCL_GBA=1 -DBN_CFG_SPRITE_TILES_MAX_ITEMS=256
USERCXXFLAGS	:=
USERASFLAGS 	:=
USERLDFLAGS 	:=
USERLIBDIRS 	:=
USERLIBS    	:=
DEFAULTLIBS 	:=
STACKTRACE		:=
USERBUILD   	:=
EXTTOOL     	:=

#---------------------------------------------------------------------------------------------------------------------
# Export absolute butano path:
#---------------------------------------------------------------------------------------------------------------------
ifndef LIBBUTANOABS
	export LIBBUTANOABS	:=	$(realpath $(LIBBUTANO))
endif

#---------------------------------------------------------------------------------------------------------------------
# Include main makefile:
#---------------------------------------------------------------------------------------------------------------------
include $(LIBBUTANOABS)/butano.mak
