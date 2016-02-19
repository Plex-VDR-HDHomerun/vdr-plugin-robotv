/*
 *      vdr-plugin-robotv - RoboTV server plugin for VDR
 *
 *      Copyright (C) 2016 Alexander Pipelka
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

#include "demuxer_ADTS.h"
#include "vdr/tools.h"
#include "aaccommon.h"

ParserAdts::ParserAdts(TsDemuxer* demuxer) : Parser(demuxer, 64 * 1024, 8192) {
    m_headerSize = 9; // header is 9 bytes long (with CRC)
}

bool ParserAdts::ParseAudioHeader(uint8_t* buffer, int& channels, int& samplerate, int& framesize) {
    cBitStream bs(buffer, m_headerSize * 8);

    // sync
    if(bs.GetBits(12) != 0xFFF) {
        return false;
    }

    bs.SkipBits(1); // MPEG Version (0 = MPEG4 / 1 = MPEG2)

    // layer if always 0
    if(bs.GetBits(2) != 0) {
        return false;
    }

    bs.SkipBits(1); // Protection absent
    bs.SkipBits(2); // AOT
    int samplerateindex = bs.GetBits(4); // sample rate index

    if(samplerateindex == 15) {
        return false;
    }

    bs.SkipBits(1);      // Private bit

    int channelindex = bs.GetBits(3); // channel index

    if(channelindex > 7) {
        return false;
    }

    bs.SkipBits(4); // original, copy, copyright, ...

    framesize = bs.GetBits(13);

    m_sampleRate = aac_samplerates[samplerateindex];
    m_channels = aac_channels[channelindex];
    m_duration = 1024 * 90000 / m_sampleRate;

    return true;
}

bool ParserAdts::checkAlignmentHeader(unsigned char* buffer, int& framesize) {
    int channels, samplerate;
    return ParseAudioHeader(buffer, channels, samplerate, framesize);
}

int ParserAdts::parsePayload(unsigned char* payload, int length) {
    int framesize = 0;

    if(!ParseAudioHeader(payload, m_channels, m_sampleRate, framesize)) {
        return length;
    }

    m_demuxer->setAudioInformation(m_channels, m_sampleRate, 0, 0, 0);
    return length;
}
