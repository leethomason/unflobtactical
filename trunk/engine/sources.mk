sources :=  camera.cpp \
			engine.cpp \
			fixedgeom.cpp \
			gpustatemanager.cpp \
			loosequadtree.cpp \
			map.cpp \
			model.cpp \
			particle.cpp \
			particleeffect.cpp \
			renderqueue.cpp \
			settings.cpp \
			screenport.cpp \
			serialize.cpp \
			surface.cpp \
			text.cpp \
			texture.cpp \
			ufoutil.cpp \
			uirendering.cpp \

			
LOCAL_SRC_FILES += $(sources:%=/../../../engine/%) 
