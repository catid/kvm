// Copyright (c) 2020 Christopher A. Taylor.  All rights reserved.

/*
    Video Parser
*/

#pragma once

#include "kvm_serializer.hpp"

#include <functional>

namespace kvm {


//------------------------------------------------------------------------------
// Tools

// Parses buffer for 00 00 01 start codes
int FindAnnexBStart(const uint8_t* data, int bytes);

using NaluCallback = std::function<void(uint8_t* data, int bytes)>;

// Invoke the callback for each Annex B NALU for H.264/H.265 video
int EnumerateAnnexBNalus(
    uint8_t* data,
    int bytes,
    NaluCallback callback);

// Read ExpGolomb format from H.264/HEVC
unsigned ReadExpGolomb(ReadBitStream& bs);


//------------------------------------------------------------------------------
// VideoParser

struct CopyRange
{
    uint8_t* Ptr = nullptr;
    int Bytes = 0;

    CopyRange()
    {
    }
    CopyRange(uint8_t* ptr, int bytes)
        : Ptr(ptr)
        , Bytes(bytes)
    {
    }
};

static const int kMaxCopyRangesPerPicture = 16;

struct PictureRanges
{
    bool Keyframe = false;
    std::vector<CopyRange> Ranges;
    int RangeCount = 0;
    int TotalBytes = 0;
};

struct VideoParser
{
    int NalUnitCount = 0;

    // Parameter data
    std::vector<CopyRange> Parameters;
    int TotalParameterBytes = 0;

    // Picture data
    std::vector<PictureRanges> Pictures;

    // Updated by AppendSlice
    int WritePictureIndex = -1;

    void Reset();

    // Parse video into parameter/picture NAL units
    void ParseVideo(
        bool is_hevc_else_h264,
        uint8_t* data,
        int bytes);

protected:
    void ParseNalUnitH264(uint8_t* data, int bytes);
    void ParseNalUnitHEVC(uint8_t* data, int bytes);
    void AppendSlice(uint8_t* ptr, int bytes, bool new_picture, bool keyframe);
};


//------------------------------------------------------------------------------
// SDP

std::string GenerateSDP(
    const void* sps_data, int sps_bytes,
    const void* pps_data, int pps_bytes,
    int dest_port);


//------------------------------------------------------------------------------
// RTP Payloader

using RtpCallback = std::function<void(const uint8_t* rtp_data, int rtp_bytes)>;

class RtpPayloader
{
public:
    // Maximum size of datagrams produced by RTP payloader
    static const int kDatagramBytes = 1200;

    RtpPayloader();

    void WrapH264Rtp(
        uint64_t shutter_usec,
        const uint8_t* data,
        int bytes,
        RtpCallback callback);

    std::string GenerateSDP() const;

protected:
    uint16_t NextSequence = 0;
    uint32_t SSRC = 0;
    uint8_t Datagram[kDatagramBytes];

    mutable std::mutex Lock;
    std::vector<uint8_t> SPS, PPS;
};


} // namespace kvm
