sources :=  glcontainer.cpp \
			gldebug.cpp \
			glfixed.cpp \
			glgeometry.cpp \
			glmatrix.cpp \
			glmemorypool.cpp \
			glperformance.cpp \
			glprime.cpp \
			glrandom.cpp \
			glstringutil.cpp \
			glutil.cpp \
			glvector.cpp \
			
LOCAL_SRC_FILES += $(sources:%=/../../../grinliz/%) 
