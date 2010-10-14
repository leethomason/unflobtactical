del version.h
echo const int VERSION= >> version.h
svnversion >> version.h
echo ; >> version.h

del .\android\ufoattack_1\src\com\grinninglizard\UFOAttack\UFOVersion.java
echo package com.grinninglizard.UFOAttack; >> .\android\ufoattack_1\src\com\grinninglizard\UFOAttack\UFOVersion.java
echo public class UFOVersion { >> .\android\ufoattack_1\src\com\grinninglizard\UFOAttack\UFOVersion.java
echo public static final int VERSION		= >> .\android\ufoattack_1\src\com\grinninglizard\UFOAttack\UFOVersion.java
svnversion >> .\android\ufoattack_1\src\com\grinninglizard\UFOAttack\UFOVersion.java
echo ;} >> .\android\ufoattack_1\src\com\grinninglizard\UFOAttack\UFOVersion.java



