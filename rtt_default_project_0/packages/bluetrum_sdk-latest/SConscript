import os
from building import * 
Import('RTT_ROOT')
Import('rtconfig')

#---------------------------------------------------------------------------------
# Package configuration
#---------------------------------------------------------------------------------
PKGNAME = "bluetrum-sdk"
VERSION = "v0.0.1"
DEPENDS = [""]

#---------------------------------------------------------------------------------
# Compile the configuration 
#
# SOURCES: Need to compile c and c++ source, auto search when SOURCES is empty
# 
# LOCAL_CPPPATH: Local file path (.h/.c/.cpp)
# LOCAL_CCFLAGS: Local compilation parameter 
# LOCAL_ASFLAGS: Local assembly parameters
# 
# CPPPATH: Global file path (.h/.c/.cpp), auto search when LOCAL_CPPPATH/CPPPATH 
#          is empty # no pass!!!
# CCFLAGS: Global compilation parameter 
# ASFLAGS: Global assembly parameters
#
# CPPDEFINES: Global macro definition
# LOCAL_CPPDEFINES: Local macro definition 
# 
# LIBS: Specify the static library that need to be linked
# LIBPATH: Specify the search directory for the library file (.lib/.a)
#
# LINKFLAGS: Link options
#---------------------------------------------------------------------------------
CWD              = GetCurrentDir()
SOURCES          = Glob("*.c")

LOCAL_CPPPATH    = [] 
LOCAL_CCFLAGS    = "" 
LOCAL_ASFLAGS    = ""

# CPPPATH          = [GetCurrentDir(), os.path.join(GetCurrentDir(), 'include')]
CPPPATH          = [CWD]
CPPPATH         += [CWD + '/blue_driver']
CCFLAGS          = "" 
ASFLAGS          = ""

CPPDEFINES       = []
LOCAL_CPPDEFINES = []

LIBS             = ['hal']
LIBPATH          = [CWD]

LINKFLAGS        = "" 

SOURCES_IGNORE   = []
CPPPATH_IGNORE   = []

if GetDepend(['PKG_USING_BLUETRUM_NIMBLE']):
    LIBS        += ['btctrl']

#---------------------------------------------------------------------------------
# Main target
#---------------------------------------------------------------------------------
objs = DefineGroup(name = PKGNAME, src = SOURCES, depend = DEPENDS, 
                   CPPPATH          = CPPPATH, 
                   CCFLAGS          = CCFLAGS, 
                   ASFLAGS          = ASFLAGS, 
                   LOCAL_CPPPATH    = LOCAL_CPPPATH, 
                   LOCAL_CCFLAGS    = LOCAL_CCFLAGS, 
                   LOCAL_ASFLAGS    = LOCAL_ASFLAGS, 
                   CPPDEFINES       = CPPDEFINES, 
                   LOCAL_CPPDEFINES = LOCAL_CPPDEFINES, 
                   LIBS             = LIBS, 
                   LIBPATH          = LIBPATH,
                   LINKFLAGS        = LINKFLAGS)  

cwd  = GetCurrentDir()
list = os.listdir(cwd)

for item in list:
    if os.path.isfile(os.path.join(CWD, item, 'SConscript')):
        objs = objs + SConscript(os.path.join(item, 'SConscript'))

Return("objs")
