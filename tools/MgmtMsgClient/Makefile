## -*- Makefile -*-
##
## User: Tomasz Kleinschmidt
## Time: 2012-06-09 16:28:49
## Makefile created by Oracle Solaris Studio.
##
## This file is generated automatically.
##


#### Compiler and tool definitions shared by all build targets #####
CCC = g++
CXX = g++
BASICOPTS = -g
CCFLAGS = $(BASICOPTS)
CXXFLAGS = $(BASICOPTS)
CCADMIN = 


# Define the target directories.
TARGETDIR_MgmtMsgClient=build


all: $(TARGETDIR_MgmtMsgClient)/MgmtMsgClient

## Target: MgmtMsgClient
OBJS_MgmtMsgClient =  \
	$(TARGETDIR_MgmtMsgClient)/OptBuffer.o \
	$(TARGETDIR_MgmtMsgClient)/Client.o \
	$(TARGETDIR_MgmtMsgClient)/Help.o \
	$(TARGETDIR_MgmtMsgClient)/Network.o \
	$(TARGETDIR_MgmtMsgClient)/MgmtMsgClient.o
USERLIBS_MgmtMsgClient = $(SYSLIBS_MgmtMsgClient) 
DEPLIBS_MgmtMsgClient =  
LDLIBS_MgmtMsgClient = $(USERLIBS_MgmtMsgClient)


# Link or archive
$(TARGETDIR_MgmtMsgClient)/MgmtMsgClient: $(TARGETDIR_MgmtMsgClient) $(OBJS_MgmtMsgClient) $(DEPLIBS_MgmtMsgClient)
	$(LINK.cc) $(CCFLAGS_MgmtMsgClient) $(CPPFLAGS_MgmtMsgClient) -o $@ $(OBJS_MgmtMsgClient) $(LDLIBS_MgmtMsgClient)


# Compile source files into .o files
$(TARGETDIR_MgmtMsgClient)/OptBuffer.o: $(TARGETDIR_MgmtMsgClient) OptBuffer.cpp
	$(COMPILE.cc) $(CCFLAGS_MgmtMsgClient) $(CPPFLAGS_MgmtMsgClient) -o $@ OptBuffer.cpp

$(TARGETDIR_MgmtMsgClient)/Client.o: $(TARGETDIR_MgmtMsgClient) Client.cpp
	$(COMPILE.cc) $(CCFLAGS_MgmtMsgClient) $(CPPFLAGS_MgmtMsgClient) -o $@ Client.cpp

$(TARGETDIR_MgmtMsgClient)/Help.o: $(TARGETDIR_MgmtMsgClient) Help.cpp
	$(COMPILE.cc) $(CCFLAGS_MgmtMsgClient) $(CPPFLAGS_MgmtMsgClient) -o $@ Help.cpp

$(TARGETDIR_MgmtMsgClient)/Network.o: $(TARGETDIR_MgmtMsgClient) Network.cpp
	$(COMPILE.cc) $(CCFLAGS_MgmtMsgClient) $(CPPFLAGS_MgmtMsgClient) -o $@ Network.cpp

$(TARGETDIR_MgmtMsgClient)/MgmtMsgClient.o: $(TARGETDIR_MgmtMsgClient) MgmtMsgClient.cpp
	$(COMPILE.cc) $(CCFLAGS_MgmtMsgClient) $(CPPFLAGS_MgmtMsgClient) -o $@ MgmtMsgClient.cpp



#### Clean target deletes all generated files ####
clean:
	rm -f \
		$(TARGETDIR_MgmtMsgClient)/MgmtMsgClient \
		$(TARGETDIR_MgmtMsgClient)/OptBuffer.o \
		$(TARGETDIR_MgmtMsgClient)/Client.o \
		$(TARGETDIR_MgmtMsgClient)/Help.o \
		$(TARGETDIR_MgmtMsgClient)/Network.o \
		$(TARGETDIR_MgmtMsgClient)/MgmtMsgClient.o
	$(CCADMIN)
	rm -f -r $(TARGETDIR_MgmtMsgClient)


# Create the target directory (if needed)
$(TARGETDIR_MgmtMsgClient):
	mkdir -p $(TARGETDIR_MgmtMsgClient)


# Enable dependency checking
.KEEP_STATE:
.KEEP_STATE_FILE:.make.state.GNU-i386-Linux

