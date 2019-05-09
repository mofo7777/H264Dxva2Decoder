//----------------------------------------------------------------------------------------------
// PlayerDefinition.h
//----------------------------------------------------------------------------------------------
#ifndef PLAYER_DEFINITION_H
#define PLAYER_DEFINITION_H

// One msec in ns
const MFTIME ONE_MSEC = 10000;
// One second in ns
const MFTIME ONE_SECOND = 10000000;
// One minute in ns
const MFTIME ONE_MINUTE = 600000000;
// One hour in ns
const MFTIME ONE_HOUR = 36000000000;
// One day in ns
const MFTIME ONE_DAY = 864000000000;

#define F_KEY	0x46
#define P_KEY	0x50
#define S_KEY	0x53

#define CALLBACK_MSG_NOT_SET	0

#define WND_MSG_PLAYING			1
#define WND_MSG_PAUSING			2
#define WND_MSG_STOPPING		3
#define WND_MSG_FINISH			4

#define MSG_PROCESS_DECODING		1
#define MSG_PROCESS_RENDERING		2
#ifdef USE_MMCSS_WORKQUEUE
#define PLAYER_MSG_REGISTER_MMCSS	3
#define PLAYER_MSG_UNREGISTER_MMCSS	4
#endif

enum PLAYER_STATE{ STATE_PLAYING, STATE_PAUSING, STATE_STOPPING };

#define WM_DXVA2_SETTINGS			WM_USER
#define WM_DXVA2_SETTINGS_RESET		WM_USER + 1

#endif