sources :=  ai.cpp \
			chits.cpp \
			cgame.cpp \
			game.cpp \
			gameresources.cpp \
			geoai.cpp \
			inventory.cpp \
			item.cpp \
			research.cpp \
			scene.cpp \
			settings.cpp \
			stats.cpp \
			tacmap.cpp \
			ufosound.cpp \
			unit.cpp \
			areawidget.cpp \
			inventoryWidget.cpp \
			storageWidget.cpp \
			basetradescene.cpp \
			battlescene.cpp \
			battlevisibility.cpp \
			buildbasescene.cpp \
			characterscene.cpp \
			dialogscene.cpp \
			fastbattlescene.cpp \
			geoendscene.cpp \
			geomap.cpp \
			geoscene.cpp \
			helpscene.cpp \
			researchscene.cpp \
			tacticalendscene.cpp \
			tacticalintroscene.cpp \
			tacticalunitscorescene.cpp \

			
LOCAL_SRC_FILES += $(sources:%=/../../../game/%) 


