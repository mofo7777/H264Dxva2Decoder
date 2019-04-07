//----------------------------------------------------------------------------------------------
// PlayerDefinition.h
//----------------------------------------------------------------------------------------------
#ifndef PLAYER_DEFINITION_H
#define PLAYER_DEFINITION_H

const DWORD READ_SIZE = 65536;

// One minute in ns
const MFTIME ONE_MINUTE = 600000000;

#define P_KEY	0x50
#define S_KEY	0x53

#define WND_MSG_NOT_SET		0
#define WND_MSG_PLAYING		1
#define WND_MSG_PAUSING		2
#define WND_MSG_STOPPING	3
#define WND_MSG_FINISH		4

#endif