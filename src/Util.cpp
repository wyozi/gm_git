#include "Util.h"

std::string Wyozi::Util::GitErrorToString(int error) {
	const git_error *e = giterr_last();
	std::ostringstream stringStream;
	stringStream << "Error: " << error << "/" << e->klass << ": " << e->message;
	return stringStream.str();
}

void Wyozi::Util::PrintMessage(lua_State* state, char* message) {
	LUA->PushSpecial(GarrysMod::Lua::SPECIAL_GLOB);
		LUA->GetField( -1, "MsgN" );
			LUA->PushString(message);
			LUA->Call(1, 0);
		LUA->Pop();
	LUA->Pop();
}