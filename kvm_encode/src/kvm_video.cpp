// Copyright (c) 2020 Christopher A. Taylor.  All rights reserved.

#include "kvm_video.hpp"
#include "kvm_logger.hpp"

#include <sstream>
using namespace std;

namespace kvm {

static logger::Channel Logger("VideoParser");


//------------------------------------------------------------------------------
// Tools

// Parses buffer for 00 00 01 start codes
int FindAnnexBStart(const uint8_t* data, int bytes)
{
    bytes -= 2;
    for (int i = 0; i < bytes; ++i)
    {
        if (data[i] == 0) {
            if (data[i + 1] == 0) {
                if (data[i + 2] == 1) {
                    return i;
                }
            }
        }
    }
    return -1;
}

static const int kAnnexBPrefixBytes = 3;

int EnumerateAnnexBNalus(uint8_t* data, int bytes, NaluCallback callback)
{
    int nalu_count = 0;
    int last_offset = -kAnnexBPrefixBytes;

    for (;;)
    {
        const int next_start = last_offset + kAnnexBPrefixBytes;
        int nal_offset = FindAnnexBStart(data + next_start, bytes - next_start);

        if (nal_offset < 0) {
            break;
        }
        nal_offset += next_start;

        if (last_offset >= 0)
        {
            uint8_t* nal_data = data + last_offset + kAnnexBPrefixBytes;
            int nal_bytes = nal_offset - last_offset - kAnnexBPrefixBytes;

            // Check for extra 00
            if (nal_data[nal_bytes - 1] == 0) {
                nal_bytes--;
            }

            callback(nal_data, nal_bytes);
            ++nalu_count;
        }

        last_offset = nal_offset;
    }

    if (last_offset >= 0)
    {
        uint8_t* nal_data = data + last_offset + kAnnexBPrefixBytes;
        const int nal_bytes = bytes - last_offset - kAnnexBPrefixBytes;

        callback(nal_data, nal_bytes);
        ++nalu_count;
    }

    return nalu_count;
}

unsigned ReadExpGolomb(ReadBitStream& bs)
{
    // Count number of leading zeroes
    unsigned read_count = 0;
    for (read_count = 0; read_count < 128; ++read_count) {
        if (bs.Read(1) != 0) {
            break;
        }
    }

    if (read_count <= 0 || read_count > 32) {
        return 0;
    }

    return bs.Read(read_count) + (1 << read_count) - 1;
}


//------------------------------------------------------------------------------
// VideoParser

void VideoParser::Reset()
{
    NalUnitCount = 0;

    Parameters.reserve(3);
    Parameters.clear();
    TotalParameterBytes = 0;

    Pictures.reserve(1);
    Pictures.clear();
    WritePictureIndex = -1;
}

void VideoParser::AppendSlice(uint8_t* ptr, int bytes, bool new_picture, bool keyframe)
{
    if (new_picture) {
        ++WritePictureIndex;
    }
    if (WritePictureIndex >= (int)Pictures.size()) {
        Pictures.resize(WritePictureIndex + 1);
    }
    if (WritePictureIndex < 0) {
        Logger.Warn("Dropping dangling NAL unit from encoder before start of picture");
        return;
    }

    auto& picture = Pictures[WritePictureIndex];

    if (picture.RangeCount >= kMaxCopyRangesPerPicture) {
        Logger.Error("FIXME: RangeCount exceeded kMaxCopyRangesPerPicture");
        return;
    }

    picture.Ranges.emplace_back(ptr, bytes);
    picture.TotalBytes += bytes;
    picture.Keyframe = keyframe;
}

void VideoParser::ParseVideo(
    bool is_hevc_else_h264,
    uint8_t* data,
    int bytes)
{
    NaluCallback nalu_callback;
    if (is_hevc_else_h264) {
        nalu_callback = [this](uint8_t* data, int bytes) {
            ParseNalUnitHEVC(data, bytes);
        };
    }
    else {
        nalu_callback = [this](uint8_t* data, int bytes) {
            ParseNalUnitH264(data, bytes);
        };
    }

    NalUnitCount += EnumerateAnnexBNalus(
        data,
        bytes,
        nalu_callback);
}

void VideoParser::ParseNalUnitH264(uint8_t* data, int bytes)
{
    if (bytes < 1) {
        Logger.Error("Encoder produced invalid truncated NALU");
        return; // Invalid
    }
    const uint8_t header = data[0];

    if ((header & 0x80) != 0) {
        Logger.Error("Encoder produced invalid highbit NALU");
        return; // Invalid
    }

    //const unsigned nal_ref_idc = (header >> 5) & 3;
    const unsigned nal_unit_type = header & 0x1f;
    //spdlog::info("NALU {} bytes nal_unit_type={} nal_ref_idc={}", bytes, nal_unit_type, nal_ref_idc);

    bool keyframe = false;
    switch (nal_unit_type)
    {
    case 7: // Fall-thru
    case 8:
        Parameters.emplace_back(data - 3, bytes + 3);
        TotalParameterBytes += bytes + 3;
        break;
    case 5:
        keyframe = true;
        // Fall-thru
    case 1:
        {
            ReadBitStream bs(data + 1, bytes - 1);
            unsigned first_mb_in_slice = ReadExpGolomb(bs);
            const bool FirstSlice = (first_mb_in_slice == 0);
            //unsigned slice_type = ReadExpGolomb(bs);
            //spdlog::info("first_mb_in_slice={} slice_type={}", first_mb_in_slice, slice_type);
            // We are at the start of a new image when first_mb_in_slice = 0

            AppendSlice(data - 3, bytes + 3, FirstSlice, keyframe);
        }
        break;
    case 9: // Ignoring AUD
        break;
    case 6: // Stripping out SEI
        // We strip SEI because this is used for the decoder to buffer up a
        // number of frames so no I-frames are needed.  However we put parameter
        // sets in front of real I-frames so SEI is not needed.
        break;
    default:
        Logger.Warn("Unhandled AVC NAL unit {} in encoder output ignored", nal_unit_type);
        break;
    }
}

void VideoParser::ParseNalUnitHEVC(uint8_t* data, int bytes)
{
    if (bytes < 2) {
        Logger.Error("Encoder produced invalid truncated NALU");
        return; // Invalid
    }
    const uint16_t header = ReadU16_BE(data);

    if ((header & 0x8000) != 0) {
        Logger.Error("Encoder produced invalid highbit NALU");
        return; // Invalid
    }

    const unsigned nal_unit_type = (header >> 9) & 0x3f;
    //const unsigned nuh_layer_id = (header >> 3) & 0x3f;
    //const unsigned nul_temporal_id = header & 7;
    //spdlog::info("NALU: {} bytes nal_unit_type={}", bytes, nal_unit_type);

    bool keyframe = false;
    switch (nal_unit_type)
    {
    case 32: // Fall-thru
    case 33: // Fall-thru
    case 34:
        Parameters.emplace_back(data - 3, bytes + 3);
        TotalParameterBytes += bytes + 3;
        break;
    case 19: // Fall-thru
    case 20:
        keyframe = true;
        // Fall-thru
    case 1: // Fall-thru
    case 21:
        {
            ReadBitStream bs(data + 2, bytes - 2);
            const bool FirstSlice = (bs.Read(1) != 0);
            //spdlog::info("FirstSlice = {}", FirstSlice);

            AppendSlice(data - 3, bytes + 3, FirstSlice, keyframe);
        }
        break;
    case 35: // Ignoring AUD
        break;
    case 39: // Stripping out SEI
        // We strip SEI because this is used for the decoder to buffer up a
        // number of frames so no I-frames are needed.  However we put parameter
        // sets in front of real I-frames so SEI is not needed.
        break;
    default:
        Logger.Warn("Unhandled HEVC NAL unit {} in encoder output ignored", nal_unit_type);
        break;
    }
}


//------------------------------------------------------------------------------
// RTP Payloader

// https://nullprogram.com/blog/2018/07/31/
// exact bias: 0.020888578919738908
uint32_t triple32(uint32_t x)
{
    x ^= x >> 17;
    x *= UINT32_C(0xed5ad4bb);
    x ^= x >> 11;
    x *= UINT32_C(0xac4c1b51);
    x ^= x >> 15;
    x *= UINT32_C(0x31848bab);
    x ^= x >> 14;
    return x;
}

static void WriteRtpHeader(
    uint8_t* dest,
    bool marked,
    uint16_t sequence_number,
    uint32_t pts,
    uint32_t ssrc)
{
    const int payload_type = 0x60;

    uint32_t word0;
    word0 = UINT32_C(0x80000000);
    //word0 |= (CC & 0x0f) << 24;
    if (marked) {
        word0 |= 1 << 23;
    }
    word0 |= (payload_type & 0x7f) << 16;
    word0 |= sequence_number;

    WriteU32_BE(dest, word0);
    WriteU32_BE(dest + 4, pts);
    WriteU32_BE(dest + 8, ssrc);
}

static const int kRtpBytes = 12;

RtpPayloader::RtpPayloader()
{
    SSRC = triple32((uint32_t)GetTimeUsec());
}

void RtpPayloader::WrapH264Rtp(
    uint64_t shutter_usec,
    const uint8_t* data,
    int bytes,
    RtpCallback callback)
{
    const uint32_t pts = static_cast<uint32_t>( shutter_usec * 9 / 100 );

    EnumerateAnnexBNalus((uint8_t*)data, bytes, [&](uint8_t* nalu_data, int nalu_bytes) {
        const int nal_ref_idc = (nalu_data[0] >> 5) & 3;
        const int nal_unit_type = nalu_data[0] & 0x1f;

        if (nal_unit_type == 7) {
            std::lock_guard<std::mutex> locker(Lock);
            SPS.resize(nalu_bytes);
            memcpy(SPS.data(), nalu_data, nalu_bytes);
        }
        if (nal_unit_type == 8) {
            std::lock_guard<std::mutex> locker(Lock);
            PPS.resize(nalu_bytes);
            memcpy(PPS.data(), nalu_data, nalu_bytes);
        }

        // M=1 if this is an access unit
        const bool marked = nal_unit_type >= 1 && nal_unit_type <= 5;

        uint8_t* dest = Datagram;

        if (nalu_bytes + kRtpBytes <= kDatagramBytes) {
            WriteRtpHeader(
                dest,
                marked,
                NextSequence++,
                pts,
                SSRC);
            memcpy(dest + kRtpBytes, nalu_data, nalu_bytes);
            callback(dest, kRtpBytes + nalu_bytes);
            return;
        }

        const int kFuOverhead = kRtpBytes + 2; // FU-A overhead

        const uint8_t* src = nalu_data + 1;
        int remaining = nalu_bytes - 1;

        bool first = true;
        while (remaining > 0) {
            int frag_bytes = kDatagramBytes - kFuOverhead;
            const bool last = remaining <= frag_bytes;
            if (last) {
                frag_bytes = remaining;
            }

            WriteRtpHeader(
                dest,
                marked && last,
                NextSequence++,
                pts,
                SSRC);

            dest[kRtpBytes] = static_cast<uint8_t>( 28 | (nal_ref_idc << 5) );

            uint8_t fu = static_cast<uint8_t>( nal_unit_type );
            if (first) {
                fu |= 0x80;
            }
            if (last) {
                fu |= 0x40;
            }
            dest[kRtpBytes + 1] = fu;

            memcpy(dest + kFuOverhead, src, frag_bytes);

            callback(dest, kFuOverhead + frag_bytes);

            src += frag_bytes;
            remaining -= frag_bytes;
            first = false;
        }
    });
}

std::string RtpPayloader::GenerateSDP() const
{
    std::lock_guard<std::mutex> locker(Lock);

    if (SPS.empty() || PPS.empty()) {
        return "";
    }

    std::vector<char> sps_b64(GetBase64LengthFromByteCount(SPS.size()) + 1);
    std::vector<char> pps_b64(GetBase64LengthFromByteCount(PPS.size()) + 1);
    WriteBase64Str(SPS.data(), SPS.size(), sps_b64.data(), sps_b64.size());
    WriteBase64Str(PPS.data(), PPS.size(), pps_b64.data(), pps_b64.size());

    uint32_t seed = (uint32_t)GetTimeUsec();
    uint64_t id = (uint64_t)triple32(seed) | ((uint64_t)triple32(seed + 12345) << 32);
    id >>= 1;

    ostringstream oss;
    oss << "v=0\r\n";
    oss << "o=- " << id << " 1 IN IP4 127.0.0.1\r\n";
    oss << "s=Mountpoint 0\r\n";
    oss << "t=0 0\r\n";
    oss << "m=video 1 RTP/SAVPF 96\r\n";
    oss << "c=IN IP4 0.0.0.0\r\n";
    oss << "a=rtpmap:96 H264/90000\r\n";
    oss << "a=fmtp:96 sprop-sps=" << sps_b64.data() << "\r\n";
    oss << "a=fmtp:96 sprop-pps=" << pps_b64.data() << "\r\n";
    oss << "a=rtcp-fb:96 nack\r\n";
    oss << "a=rtcp-fb:96 nack pli\r\n";
    oss << "a=rtcp-fb:96 goog-remb\r\n";
    oss << "a=sendonly\r\n";
    //oss << "a=extmap:1 urn:ietf:params:rtp-hdrext:sdes:mid";
    return oss.str();
}


} // namespace kvm
