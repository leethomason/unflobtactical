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
			gamesettings.cpp \
			stats.cpp \
			tacmap.cpp \
			ufosound.cpp \
			unit.cpp \
			areawidget.cpp \
			inventoryWidget.cpp \
			firewidget.cpp \
			storageWidget.cpp \
			basetradescene.cpp \
			battledata.cpp \
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
			newgeooptions.cpp \
			newtacticaloptions.cpp \
			researchscene.cpp \
			saveloadscene.cpp \
			settingscene.cpp \
			tacticalendscene.cpp \
			tacticalintroscene.cpp \
			tacticalunitscorescene.cpp \

			
LOCAL_SRC_FILES += $(sources:%=/../../../game/%) 


