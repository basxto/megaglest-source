// ==============================================================
//	This file is part of MegaGlest Shared Library (www.glest.org)
//
//	Copyright (C) 2009-2010 Titus Tscharntke (info@titusgames.de) and
//                          Mark Vejvoda (mark_vejvoda@hotmail.com)
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================
#ifndef _SHARED_PLATFORMCOMMON_IRCTHREAD_H_
#define _SHARED_PLATFORMCOMMON_IRCTHREAD_H_

#ifdef WIN32
    #include <winsock2.h>
#endif

#include "base_thread.h"
#include <vector>
#include <string>

#include "leak_dumper.h"

// Special way to forward declare a typedef struct
struct irc_session_s;
typedef struct irc_session_s irc_session_t;
//

using namespace std;

namespace Shared { namespace PlatformCommon {

// =====================================================
//	class IRCThreadThread
// =====================================================

enum IRCEventType {
    IRC_evt_chatText = 0,
    IRC_evt_exitThread = 1
};

void normalizeNick(char *nick);

class IRCCallbackInterface {
public:
    virtual void IRC_CallbackEvent(IRCEventType evt, const char* origin, const char **params, unsigned int count) = 0;

    virtual ~IRCCallbackInterface() {}
};

class IRCThread : public BaseThread
{
public:
	static bool debugEnabled;
protected:
	static const char *globalCacheContainerName;

    std::vector<string> argv;

    Mutex mutexIRCSession;
    irc_session_t *ircSession;

    string execute_cmd_onconnect;
    //string password;
    string username;
	string channel;
	string nick;

	string playerName;

    bool hasJoinedChannel;

    Mutex mutexEventDataDone;
    bool eventDataDone;

    Mutex mutexNickList;
    time_t lastNickListUpdate;
    std::vector<string> eventData;

    Mutex mutexIRCCB;
    IRCCallbackInterface *callbackObj;

    bool wantToLeaveChannel;

    int irc_run_session(irc_session_t * session);

public:

	IRCThread(const std::vector<string> &argv,IRCCallbackInterface *callbackObj);
	virtual ~IRCThread();
    virtual void execute();
    virtual void signalQuit();
    virtual bool shutdownAndWait();

    static void setGlobalCacheContainerName(const char *name) { globalCacheContainerName = name; }

    void setPlayerName(string value) { playerName = value; }
    string getPlayerName() const { return playerName; }

    bool getWantToLeaveChannel() const { return wantToLeaveChannel; }

    void SendIRCCmdMessage(string target, string msg);
    std::vector<string> getNickList();
    bool isConnected(bool mutexLockRequired=true);

    std::vector<string> GetIRCConnectedNickList(string target, bool waitForCompletion);

    bool getEventDataDone();
    void setEventDataDone(bool value);

    bool getHasJoinedChannel() const { return hasJoinedChannel; }
    void setHasJoinedChannel(bool value) { hasJoinedChannel=value; }

    time_t getLastNickListUpdate() const { return lastNickListUpdate; }
    void setLastNickListUpdate(time_t value) { lastNickListUpdate = value;}

	string getChannel() const { return channel;}
	string getNick() const { return nick;}

	string getExecute_cmd_onconnect() const { return execute_cmd_onconnect; }
	void setExecute_cmd_onconnect(string value) { execute_cmd_onconnect = value; }

    std::vector<string> getArgs() const { return argv;}

    Mutex * getMutexNickList() { return &mutexNickList; }
    std::vector<string> & getCachedNickList() { return eventData; }
    void setCachedNickList(std::vector<string> &list) { eventData = list; }

    Mutex * getMutexIRCCB() { return &mutexIRCCB; }
    IRCCallbackInterface * getCallbackObj(bool lockObj=true);
    void setCallbackObj(IRCCallbackInterface *cb);

    void joinChannel();
    void leaveChannel();
    void connectToHost();
    void disconnect();
};

}}//end namespace

#endif
