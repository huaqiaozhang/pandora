PACKAGE=runPandora

CC=g++

INCLUDES = Makefile

BASEDIR  = ./
pandoraDIR  = /afs/cern.ch/work/h/huaqiao/CMS/ILCSoftwareSetup/PandoraPFANew/v00-09

SRCDIR   = $(BASEDIR)/src
INCDIR   = $(BASEDIR)/include
OBJDIR   = $(BASEDIR)/tmp
EXEDIR   = $(BASEDIR)/bin

TARGET   = $(EXEDIR)/$(PACKAGE)

SKIPLIST = RootDictionary.cxx 
CCLIST   = $(filter-out $(SKIPLIST),$(patsubst $(SRCDIR)/%cxx, %cxx, $(wildcard $(SRCDIR)/*.cxx)))
CCLIST   += RootDictionary.cxx

OBJECTS  = $(patsubst %.cxx,%.o,$(CCLIST))

vpath %.h   $(INCDIR) $(SRCDIR)
vpath %.cxx $(SRCDIR)
vpath %.o   $(OBJDIR)

INCFLAGS = -g -I$(INCDIR) -fPIC -O -DSTANDALONE 
INCFLAGS += $(shell root-config --cflags) -O -I$(pandoraDIR)/include

LIBS  = $(shell root-config --ldflags) -lXMLIO -lXMLParser  -g  
LIBS += $(shell root-config --libs) 
LIBS += -L$(pandoraDIR)/lib -lPandoraFramework -lPandoraFineGranularityContent

.PHONY: all
$(TARGET): dirs $(LIB_LIST) $(OBJECTS) $(INCLUDES)
	@echo $(CCLIST)
	@echo "**"
	@echo "** Linking Executable"
	@echo "**"
	$(CC) $(addprefix $(OBJDIR)/, $(notdir $(OBJECTS))) $(addprefix $(OBJDIR)/, ) $(LIBS) -o $(TARGET)

dirs:
	@mkdir -p $(EXEDIR)
	@mkdir -p $(OBJDIR)

clean:
	@rm -rf $(OBJDIR)
	@rm -f $(SRCDIR)/*~
	@rm -f $(SRCDIR)/.*.swp

veryclean: clean
	@rm -f $(SRCDIR)/RootDictionary.*
	@rm -f $(SRCDIR)/*.root
	@rm -f $(SRCDIR)/*_hist.h; rm -f $(SRCDIR)/*_hist.cxx
	@rm -rf $(EXEDIR)

%.o: %.cxx 
	@echo "**"
	@echo "** Compiling C++ Source" 
	@echo "**"
	$(CC) -c $(INCFLAGS) $< -o $(OBJDIR)/$(notdir $@)

RootDictionary.cxx: LinkDef.h
	@echo "**"
	@echo "** Creating Root dictionary"
	@echo "**"
	rootcint -f $(SRCDIR)/RootDictionary.cxx -c $<

# Hack to avoid need to re-run make file following code generation target.
RootDictionary.o: RootDictionary.cxx
	@echo "**"
	@echo "** Compiling C++ Source" 
	@echo "**"
	$(CC) -c $(INCFLAGS)  $(SRCDIR)/RootDictionary.cxx -o $(OBJDIR)/$(notdir $@)

