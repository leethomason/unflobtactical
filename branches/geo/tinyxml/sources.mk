sources :=  tinystr.cpp \
			tinyxml.cpp \
			tinyxmlerror.cpp \
			tinyxmlparser.cpp \
			
LOCAL_SRC_FILES += $(sources:%=/../../../tinyxml/%) 

# removed:
# glperformance

