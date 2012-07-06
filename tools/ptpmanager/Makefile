##
## Auto Generated makefile by CodeLite IDE
##
## Debug
ProjectName            :=ptpmanager
IntermediateDirectory  :=./Build
OutDir                 := $(IntermediateDirectory)
CurrentFileName        :=
CurrentFilePath        :=
CurrentFileFullPath    :=
User                   :=himanshu
Date                   :=Thursday 05 July 2012
LinkerName             :=gcc
ArchiveTool            :=ar rcus
SharedObjectLinkerName :=gcc -shared -fPIC
ObjectSuffix           :=.o
DependSuffix           :=.o.d
PreprocessSuffix       :=.o.i
DebugSwitch            :=-g 
IncludeSwitch          :=-I
LibrarySwitch          :=-l
OutputSwitch           :=-o 
LibraryPathSwitch      :=-L
PreprocessorSwitch     :=-D
SourceSwitch           :=-c 
CompilerName           :=gcc
C_CompilerName         :=gcc
OutputFile             :=$(IntermediateDirectory)/$(ProjectName)
Preprocessors          :=
ObjectSwitch           :=-o 
ArchiveOutputSwitch    := 
PreprocessOnlySwitch   :=-E 
MakeDirCommand         :=mkdir -p
CmpOptions             := -g $(Preprocessors)
LinkOptions            :=  
IncludePath            :=  "$(IncludeSwitch)." "$(IncludeSwitch)." 
RcIncludePath          :=
Libs                   :=
LibPath                := "$(LibraryPathSwitch)." 


##
## User defined environment variables
##
Objects=$(IntermediateDirectory)/ptpmanager$(ObjectSuffix) $(IntermediateDirectory)/network$(ObjectSuffix) $(IntermediateDirectory)/show$(ObjectSuffix) $(IntermediateDirectory)/packOutgoingMsg$(ObjectSuffix) $(IntermediateDirectory)/handleIncomingMsg$(ObjectSuffix) 

##
## Main Build Targets 
##
all: $(OutputFile)

$(OutputFile): makeDirStep $(Objects)
	@$(MakeDirCommand) $(@D)
	$(LinkerName) $(OutputSwitch)$(OutputFile) $(Objects) $(LibPath) $(Libs) $(LinkOptions)

makeDirStep:
	@test -d ./Build || $(MakeDirCommand) ./Build

PreBuild:


##
## Objects
##
$(IntermediateDirectory)/ptpmanager$(ObjectSuffix): ptpmanager.c $(IntermediateDirectory)/ptpmanager$(DependSuffix)
	$(C_CompilerName) $(SourceSwitch) "ptpmanager.c" $(CmpOptions) $(ObjectSwitch)$(IntermediateDirectory)/ptpmanager$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/ptpmanager$(DependSuffix): ptpmanager.c
	@$(C_CompilerName) $(CmpOptions) $(IncludePath) -MT$(IntermediateDirectory)/ptpmanager$(ObjectSuffix) -MF$(IntermediateDirectory)/ptpmanager$(DependSuffix) -MM "ptpmanager.c"

$(IntermediateDirectory)/ptpmanager$(PreprocessSuffix): ptpmanager.c
	@$(C_CompilerName) $(CmpOptions) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/ptpmanager$(PreprocessSuffix) "ptpmanager.c"

$(IntermediateDirectory)/network$(ObjectSuffix): network.c $(IntermediateDirectory)/network$(DependSuffix)
	$(C_CompilerName) $(SourceSwitch) "network.c" $(CmpOptions) $(ObjectSwitch)$(IntermediateDirectory)/network$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/network$(DependSuffix): network.c
	@$(C_CompilerName) $(CmpOptions) $(IncludePath) -MT$(IntermediateDirectory)/network$(ObjectSuffix) -MF$(IntermediateDirectory)/network$(DependSuffix) -MM "network.c"

$(IntermediateDirectory)/network$(PreprocessSuffix): network.c
	@$(C_CompilerName) $(CmpOptions) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/network$(PreprocessSuffix) "network.c"

$(IntermediateDirectory)/show$(ObjectSuffix): show.c $(IntermediateDirectory)/show$(DependSuffix)
	$(C_CompilerName) $(SourceSwitch) "show.c" $(CmpOptions) $(ObjectSwitch)$(IntermediateDirectory)/show$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/show$(DependSuffix): show.c
	@$(C_CompilerName) $(CmpOptions) $(IncludePath) -MT$(IntermediateDirectory)/show$(ObjectSuffix) -MF$(IntermediateDirectory)/show$(DependSuffix) -MM "show.c"

$(IntermediateDirectory)/show$(PreprocessSuffix): show.c
	@$(C_CompilerName) $(CmpOptions) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/show$(PreprocessSuffix) "show.c"

$(IntermediateDirectory)/packOutgoingMsg$(ObjectSuffix): packOutgoingMsg.c $(IntermediateDirectory)/packOutgoingMsg$(DependSuffix)
	$(C_CompilerName) $(SourceSwitch) "packOutgoingMsg.c" $(CmpOptions) $(ObjectSwitch)$(IntermediateDirectory)/packOutgoingMsg$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/packOutgoingMsg$(DependSuffix): packOutgoingMsg.c
	@$(C_CompilerName) $(CmpOptions) $(IncludePath) -MT$(IntermediateDirectory)/packOutgoingMsg$(ObjectSuffix) -MF$(IntermediateDirectory)/packOutgoingMsg$(DependSuffix) -MM "packOutgoingMsg.c"

$(IntermediateDirectory)/packOutgoingMsg$(PreprocessSuffix): packOutgoingMsg.c
	@$(C_CompilerName) $(CmpOptions) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/packOutgoingMsg$(PreprocessSuffix) "packOutgoingMsg.c"

$(IntermediateDirectory)/handleIncomingMsg$(ObjectSuffix): handleIncomingMsg.c $(IntermediateDirectory)/handleIncomingMsg$(DependSuffix)
	$(C_CompilerName) $(SourceSwitch) "handleIncomingMsg.c" $(CmpOptions) $(ObjectSwitch)$(IntermediateDirectory)/handleIncomingMsg$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/handleIncomingMsg$(DependSuffix): handleIncomingMsg.c
	@$(C_CompilerName) $(CmpOptions) $(IncludePath) -MT$(IntermediateDirectory)/handleIncomingMsg$(ObjectSuffix) -MF$(IntermediateDirectory)/handleIncomingMsg$(DependSuffix) -MM "handleIncomingMsg.c"

$(IntermediateDirectory)/handleIncomingMsg$(PreprocessSuffix): handleIncomingMsg.c
	@$(C_CompilerName) $(CmpOptions) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/handleIncomingMsg$(PreprocessSuffix) "handleIncomingMsg.c"


-include $(IntermediateDirectory)/*$(DependSuffix)
##
## Clean
##
clean:
	$(RM) $(IntermediateDirectory)/ptpmanager$(ObjectSuffix)
	$(RM) $(IntermediateDirectory)/ptpmanager$(DependSuffix)
	$(RM) $(IntermediateDirectory)/ptpmanager$(PreprocessSuffix)
	$(RM) $(IntermediateDirectory)/network$(ObjectSuffix)
	$(RM) $(IntermediateDirectory)/network$(DependSuffix)
	$(RM) $(IntermediateDirectory)/network$(PreprocessSuffix)
	$(RM) $(IntermediateDirectory)/show$(ObjectSuffix)
	$(RM) $(IntermediateDirectory)/show$(DependSuffix)
	$(RM) $(IntermediateDirectory)/show$(PreprocessSuffix)
	$(RM) $(IntermediateDirectory)/packOutgoingMsg$(ObjectSuffix)
	$(RM) $(IntermediateDirectory)/packOutgoingMsg$(DependSuffix)
	$(RM) $(IntermediateDirectory)/packOutgoingMsg$(PreprocessSuffix)
	$(RM) $(IntermediateDirectory)/handleIncomingMsg$(ObjectSuffix)
	$(RM) $(IntermediateDirectory)/handleIncomingMsg$(DependSuffix)
	$(RM) $(IntermediateDirectory)/handleIncomingMsg$(PreprocessSuffix)
	$(RM) $(OutputFile)


