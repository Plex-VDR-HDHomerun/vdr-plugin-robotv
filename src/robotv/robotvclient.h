/*
 *      vdr-plugin-robotv - RoboTV server plugin for VDR
 *
 *      Copyright (C) 2015 Alexander Pipelka
 *
 *      https://github.com/pipelka/vdr-plugin-robotv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#ifndef ROBOTV_CLIENT_H
#define ROBOTV_CLIENT_H

#include <map>
#include <list>
#include <string>
#include <queue>
#include <set>

#include <vdr/thread.h>
#include <vdr/tools.h>
#include <vdr/receiver.h>
#include <vdr/status.h>

#include "demuxer/streaminfo.h"
#include "scanner/wirbelscan.h"
#include "recordings/artwork.h"

class cChannel;
class cDevice;
class LiveStreamer;
class MsgPacket;
class PacketPlayer;
class cCmdControl;

class RoboTVClient : public cThread, public cStatus {
private:

    unsigned int      m_Id;
    int               m_socket;
    bool              m_loggedIn;
    bool              m_StatusInterfaceEnabled;
    LiveStreamer*    m_Streamer;
    PacketPlayer*    m_RecPlayer;
    MsgPacket*        m_req;
    MsgPacket*        m_resp;
    cCharSetConv      m_toUTF8;
    uint32_t          m_protocolVersion;
    cMutex            m_msgLock;
    cMutex            m_streamerLock;
    int               m_compressionLevel;
    int               m_LanguageIndex;
    StreamInfo::Type m_LangStreamType;
    std::list<int>    m_caids;
    bool              m_wantfta;
    bool              m_filterlanguage;
    int               m_channelCount;
    int               m_timeout;
    cWirbelScan       m_scanner;
    bool              m_scanSupported;
    std::string       m_clientName;
    Artwork          m_artwork;

    std::queue<MsgPacket*> m_queue;
    cMutex                 m_queueLock;

protected:

    bool processRequest();

    virtual void Action(void);

    virtual void TimerChange(const cTimer* Timer, eTimerChange Change);
    virtual void ChannelChange(const cChannel* Channel);
    virtual void Recording(const cDevice* Device, const char* Name, const char* FileName, bool On);
    virtual void OsdStatusMessage(const char* Message);

public:

    RoboTVClient(int fd, unsigned int id);
    virtual ~RoboTVClient();

    void ChannelsChanged();
    void RecordingsChange();
    void TimerChange();

    void QueueMessage(MsgPacket* p);
    void StatusMessage(const char* Message);

    unsigned int GetID() {
        return m_Id;
    }
    const std::string& GetClientName() {
        return m_clientName;
    }
    int GetSocket() {
        return m_socket;
    }

protected:

    void SetLoggedIn(bool yesNo) {
        m_loggedIn = yesNo;
    }
    void SetStatusInterface(bool yesNo) {
        m_StatusInterfaceEnabled = yesNo;
    }
    int StartChannelStreaming(const cChannel* channel, uint32_t timeout, int32_t priority, bool waitforiframe = false, bool rawPTS = false);
    void StopChannelStreaming();

private:

    typedef struct {
        bool automatic;
        bool radio;
        std::string name;
    } ChannelGroup;

    std::map<std::string, ChannelGroup> m_channelgroups[2];

    void PutTimer(cTimer* timer, MsgPacket* p);
    bool IsChannelWanted(cChannel* channel, int type = 0);
    int  ChannelsCount();
    cString CreateLogoURL(const cChannel* channel);
    cString CreateServiceReference(const cChannel* channel);
    void addChannelToPacket(const cChannel*, MsgPacket*);

    bool process_Login();
    bool process_GetTime();
    bool process_EnableStatusInterface();
    bool process_UpdateChannels();
    bool process_ChannelFilter();

    bool processChannelStream_Open();
    bool processChannelStream_Close();
    bool processChannelStream_Pause();
    bool processChannelStream_Request();
    bool processChannelStream_Signal();

    bool processRecStream_Open();
    bool processRecStream_Close();
    bool processRecStream_GetBlock();
    bool processRecStream_GetPacket();
    bool processRecStream_Update();
    bool processRecStream_Seek();

    bool processCHANNELS_GroupsCount();
    bool processCHANNELS_ChannelsCount();
    bool processCHANNELS_GroupList();
    bool processCHANNELS_GetChannels();
    bool processCHANNELS_GetGroupMembers();

    void CreateChannelGroups(bool automatic);

    bool processTIMER_GetCount();
    bool processTIMER_Get();
    bool processTIMER_GetList();
    bool processTIMER_Add();
    bool processTIMER_Delete();
    bool processTIMER_Update();

    bool processRECORDINGS_GetDiskSpace();
    bool processRECORDINGS_GetCount();
    bool processRECORDINGS_GetList();
    bool processRECORDINGS_GetInfo();
    bool processRECORDINGS_Rename();
    bool processRECORDINGS_Delete();
    bool processRECORDINGS_Move();
    bool processRECORDINGS_SetPlayCount();
    bool processRECORDINGS_SetPosition();
    bool processRECORDINGS_GetPosition();
    bool processRECORDINGS_GetMarks();
    bool processRECORDINGS_SetUrls();

    bool processArtwork_Get();
    bool processArtwork_Set();

    bool processEPG_GetForChannel();

    bool processSCAN_ScanSupported();
    bool processSCAN_GetSetup();
    bool processSCAN_SetSetup();
    bool processSCAN_Start();
    bool processSCAN_Stop();
    bool processSCAN_GetStatus();

    void SendScannerStatus();
};

#endif // ROBOTV_CLIENT_H
