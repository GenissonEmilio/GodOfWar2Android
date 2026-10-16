# overrides/

Parches específicos de GoW II: funciones del juego que se sustituyen por C++ escrito a mano (`PS2_REGISTER_GAME_OVERRIDE`). `recompile.ps1` copia los `*.cpp` y `*.h` de aquí al runner.

El modelo es `referencia/gow1/overrides/gow_overrides.cpp` (Waffle565, vía KIexster): por ejemplo `gowIpuInit` (el `sceIpuInit` genérico del runtime trae direcciones de otro juego), el arranque, el CD o el mando. Cada uno hay que rehacerlo con **las direcciones de GoW II**, y solo cuando un bloqueo lo pida.
