## -*- Makefile -*-
##
## User: Tomasz Kleinschmidt
## Time: 2012-08-18 02:48:42
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
	$(TARGETDIR_MgmtMsgClient)/network.o \
	$(TARGETDIR_MgmtMsgClient)/OptBuffer.o \
	$(TARGETDIR_MgmtMsgClient)/client.o \
	$(TARGETDIR_MgmtMsgClient)/help.o \
	$(TARGETDIR_MgmtMsgClient)/OutgoingManagementMessage.o \
	$(TARGETDIR_MgmtMsgClient)/display.o \
	$(TARGETDIR_MgmtMsgClient)/freeing.o \
	$(TARGETDIR_MgmtMsgClient)/IncomingManagementMessage.o \
	$(TARGETDIR_MgmtMsgClient)/SequnceIdConfiguration.o \
	$(TARGETDIR_MgmtMsgClient)/MgmtMsgClient.o
USERLIBS_MgmtMsgClient = $(SYSLIBS_MgmtMsgClient) 
DEPLIBS_MgmtMsgClient =  
LDLIBS_MgmtMsgClient = $(USERLIBS_MgmtMsgClient)


# Link or archive
$(TARGETDIR_MgmtMsgClient)/MgmtMsgClient: $(TARGETDIR_MgmtMsgClient) $(OBJS_MgmtMsgClient) $(DEPLIBS_MgmtMsgClient)
	$(LINK.cc) $(CCFLAGS_MgmtMsgClient) $(CPPFLAGS_MgmtMsgClient) -o $@ $(OBJS_MgmtMsgClient) $(LDLIBS_MgmtMsgClient)


# Compile source files into .o files
$(TARGETDIR_MgmtMsgClient)/network.o: $(TARGETDIR_MgmtMsgClient) network.cpp
	$(COMPILE.cc) $(CCFLAGS_MgmtMsgClient) $(CPPFLAGS_MgmtMsgClient) -o $@ network.cpp

$(TARGETDIR_MgmtMsgClient)/OptBuffer.o: $(TARGETDIR_MgmtMsgClient) OptBuffer.cpp
	$(COMPILE.cc) $(CCFLAGS_MgmtMsgClient) $(CPPFLAGS_MgmtMsgClient) -o $@ OptBuffer.cpp

$(TARGETDIR_MgmtMsgClient)/client.o: $(TARGETDIR_MgmtMsgClient) client.cpp
	$(COMPILE.cc) $(CCFLAGS_MgmtMsgClient) $(CPPFLAGS_MgmtMsgClient) -o $@ client.cpp

$(TARGETDIR_MgmtMsgClient)/help.o: $(TARGETDIR_MgmtMsgClient) help.cpp
	$(COMPILE.cc) $(CCFLAGS_MgmtMsgClient) $(CPPFLAGS_MgmtMsgClient) -o $@ help.cpp

$(TARGETDIR_MgmtMsgClient)/OutgoingManagementMessage.o: $(TARGETDIR_MgmtMsgClient) OutgoingManagementMessage.cpp
	$(COMPILE.cc) $(CCFLAGS_MgmtMsgClient) $(CPPFLAGS_MgmtMsgClient) -o $@ OutgoingManagementMessage.cpp

$(TARGETDIR_MgmtMsgClient)/display.o: $(TARGETDIR_MgmtMsgClient) display.cpp
	$(COMPILE.cc) $(CCFLAGS_MgmtMsgClient) $(CPPFLAGS_MgmtMsgClient) -o $@ display.cpp

$(TARGETDIR_MgmtMsgClient)/freeing.o: $(TARGETDIR_MgmtMsgClient) freeing.cpp
	$(COMPILE.cc) $(CCFLAGS_MgmtMsgClient) $(CPPFLAGS_MgmtMsgClient) -o $@ freeing.cpp

$(TARGETDIR_MgmtMsgClient)/IncomingManagementMessage.o: $(TARGETDIR_MgmtMsgClient) IncomingManagementMessage.cpp
	$(COMPILE.cc) $(CCFLAGS_MgmtMsgClient) $(CPPFLAGS_MgmtMsgClient) -o $@ IncomingManagementMessage.cpp

$(TARGETDIR_MgmtMsgClient)/SequnceIdConfiguration.o: $(TARGETDIR_MgmtMsgClient) SequnceIdConfiguration.cpp
	$(COMPILE.cc) $(CCFLAGS_MgmtMsgClient) $(CPPFLAGS_MgmtMsgClient) -o $@ SequnceIdConfiguration.cpp

$(TARGETDIR_MgmtMsgClient)/MgmtMsgClient.o: $(TARGETDIR_MgmtMsgClient) MgmtMsgClient.cpp
	$(COMPILE.cc) $(CCFLAGS_MgmtMsgClient) $(CPPFLAGS_MgmtMsgClient) -o $@ MgmtMsgClient.cpp



#### Clean target deletes all generated files ####
clean:
	rm -f \
		$(TARGETDIR_MgmtMsgClient)/MgmtMsgClient \
		$(TARGETDIR_MgmtMsgClient)/network.o \
		$(TARGETDIR_MgmtMsgClient)/OptBuffer.o \
		$(TARGETDIR_MgmtMsgClient)/client.o \
		$(TARGETDIR_MgmtMsgClient)/help.o \
		$(TARGETDIR_MgmtMsgClient)/OutgoingManagementMessage.o \
		$(TARGETDIR_MgmtMsgClient)/display.o \
		$(TARGETDIR_MgmtMsgClient)/freeing.o \
		$(TARGETDIR_MgmtMsgClient)/IncomingManagementMessage.o \
		$(TARGETDIR_MgmtMsgClient)/SequnceIdConfiguration.o \
		$(TARGETDIR_MgmtMsgClient)/MgmtMsgClient.o
	$(CCADMIN)
	rm -f -r $(TARGETDIR_MgmtMsgClient)


# Create the target directory (if needed)
$(TARGETDIR_MgmtMsgClient):
	mkdir -p $(TARGETDIR_MgmtMsgClient)


# Enable dependency checking
.KEEP_STATE:
.KEEP_STATE_FILE:.make.state.GNU-i386-Linux

