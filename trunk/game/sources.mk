sources :=  ai.cpp \
			cgame.cpp \
			game.cpp \
			gameresources.cpp \
			inventory.cpp \
			inventoryWidget.cpp \
			item.cpp \
			scene.cpp \
			settings.cpp \
			stats.cpp \
			storageWidget.cpp \
			ufosound.cpp \
			unit.cpp \
			battlescene.cpp \
			battlevisibility.cpp \
			characterscene.cpp \
			dialogscene.cpp \
			helpscene.cpp \
			tacticalendscene.cpp \
			tacticalintroscene.cpp \
			tacticalunitscorescene.cpp \

			
LOCAL_SRC_FILES += $(sources:%=/../../../game/%) 


