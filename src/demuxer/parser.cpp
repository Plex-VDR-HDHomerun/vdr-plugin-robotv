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

#include "parser.h"
#include "config/config.h"
#include "vdr/tools.h"
#include "pes.h"

cParser::cParser(cTSDemuxer* demuxer, int buffersize, int packetsize) : cRingBufferLinear(buffersize, packetsize), m_demuxer(demuxer), m_startup(true) {
    m_samplerate = 0;
    m_bitrate = 0;
    m_channels = 0;
    m_duration = 0;
    m_headersize = 0;
    m_frametype = cStreamInfo::ftUNKNOWN;

    m_curPTS = DVD_NOPTS_VALUE;
    m_curDTS = DVD_NOPTS_VALUE;

    m_lastPTS = DVD_NOPTS_VALUE;
    m_lastDTS = DVD_NOPTS_VALUE;
}

cParser::~cParser() {
}

int cParser::ParsePESHeader(uint8_t* buf, size_t len) {
    // parse PES header
    unsigned int hdr_len = PesPayloadOffset(buf);

    // PTS / DTS
    int64_t pts = PesHasPts(buf) ? PesGetPts(buf) : DVD_NOPTS_VALUE;
    int64_t dts = PesHasDts(buf) ? PesGetDts(buf) : DVD_NOPTS_VALUE;

    if(dts == DVD_NOPTS_VALUE) {
        dts = pts;
    }

    if(m_curDTS == DVD_NOPTS_VALUE) {
        m_curDTS = dts;
    }

    if(m_curPTS == DVD_NOPTS_VALUE) {
        m_curPTS = pts;
    }

    return hdr_len;
}

void cParser::SendPayload(unsigned char* payload, int length) {
    sStreamPacket pkt;
    pkt.data      = payload;
    pkt.size      = length;
    pkt.duration  = m_duration;
    pkt.dts       = m_curDTS;
    pkt.pts       = m_curPTS;
    pkt.frametype = m_frametype;

    m_demuxer->SendPacket(&pkt);
}

void cParser::PutData(unsigned char* data, int length, bool pusi) {
    // get PTS / DTS on PES start
    if(pusi) {
        int offset = ParsePESHeader(data, length);
        data += offset;
        length -= offset;

        m_startup = false;
    }

    // put data
    if(!m_startup && length > 0 && data != NULL) {
        int put = Put(data, length);

        // reset buffer on overflow
        if(put < length) {
            ERRORLOG("Parser buffer overflow - resetting");
            Clear();
        }
    }
}

void cParser::Parse(unsigned char* data, int datasize, bool pusi) {
    // get available data
    int length = 0;
    uint8_t* buffer = Get(length);

    // do we have a sync ?
    int framesize = 0;

    if(length > m_headersize && buffer != NULL && CheckAlignmentHeader(buffer, framesize)) {
        // valid framesize ?
        if(framesize > 0 && length >= framesize + m_headersize) {

            // check for the next frame (eliminate false positive header checks)
            int next_framesize = 0;

            if(!CheckAlignmentHeader(&buffer[framesize], next_framesize)) {
                ERRORLOG("next frame not found on expected position, searching ...");
            }
            else {
                // check if we should extrapolate the timestamps
                if(m_curPTS == DVD_NOPTS_VALUE) {
                    m_curPTS = PtsAdd(m_lastPTS, m_duration);
                }

                if(m_curDTS == DVD_NOPTS_VALUE) {
                    m_curDTS = PtsAdd(m_lastDTS, m_duration);
                }

                int length = ParsePayload(buffer, framesize);
                SendPayload(buffer, length);

                // keep last timestamp
                m_lastPTS = m_curPTS;
                m_lastDTS = m_curDTS;

                // reset timestamps
                m_curPTS = DVD_NOPTS_VALUE;
                m_curDTS = DVD_NOPTS_VALUE;

                Del(framesize);
                PutData(data, datasize, pusi);
                return;
            }
        }

    }

    // try to find sync
    int offset = FindAlignmentOffset(buffer, length, 1, framesize);

    if(offset != -1) {
        INFOLOG("sync found at offset %i (streamtype: %s / %i bytes in buffer / framesize: %i bytes)", offset, m_demuxer->TypeName(), Available(), framesize);
        Del(offset);
    }
    else if(length > m_headersize) {
        Del(length - m_headersize);
    }

    PutData(data, datasize, pusi);
}

int cParser::ParsePayload(unsigned char* payload, int length) {
    return length;
}

int cParser::FindAlignmentOffset(unsigned char* buffer, int buffersize, int o, int& framesize) {
    framesize = 0;

    // seek sync
    while(o < (buffersize - m_headersize) && !CheckAlignmentHeader(buffer + o, framesize)) {
        o++;
    }

    // not found
    if(o >= buffersize - m_headersize || framesize <= 0) {
        return -1;
    }

    return o;
}

bool cParser::CheckAlignmentHeader(unsigned char* buffer, int& framesize) {
    framesize = 0;
    return true;
}

int cParser::FindStartCode(unsigned char* buffer, int buffersize, int offset, uint32_t startcode, uint32_t mask) {
    uint32_t sc = 0xFFFFFFFF;

    while(offset < buffersize) {

        sc = (sc << 8) | buffer[offset++];

        if((uint32_t)(sc & mask) == startcode) {
            return offset - 4;
        }
    }

    return -1;
}
