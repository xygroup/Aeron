/*
 * Copyright 2014 - 2016 Real Logic Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef INCLUDED_AERON_CNC_FILE_DESCRIPTOR__
#define INCLUDED_AERON_CNC_FILE_DESCRIPTOR__

#include "util/Index.h"
#include "concurrent/AtomicBuffer.h"
#include "util/MemoryMappedFile.h"

namespace aeron {

using namespace aeron::util;
using namespace aeron::concurrent;

/**
* Description of the command and control file used between driver and clients
*
* File Layout
* <pre>
*  +----------------------------+
*  |      Aeron CnC Version     |
*  +----------------------------+
*  |          Meta Data         |
*  +----------------------------+
*  |      to-driver Buffer      |
*  +----------------------------+
*  |      to-clients Buffer     |
*  +----------------------------+
*  |   Counter Metadata Buffer  |
*  +----------------------------+
*  |    Counter Values Buffer   |
*  +----------------------------+
*  |          Error Log         |
*  +----------------------------+
* </pre>
*
* Meta Data Layout (CnC Version 4)
* <pre>
*  +----------------------------+
*  |   to-driver buffer length  |
*  +----------------------------+
*  |  to-clients buffer length  |
*  +----------------------------+
*  |   metadata buffer length   |
*  +----------------------------+
*  |    values buffer length    |
*  +----------------------------+
*  |   Client Liveness Timeout  |
*  |                            |
*  +----------------------------+
*  |      Error Log length      |
*  +----------------------------+
* </pre>
*/
namespace CncFileDescriptor {

static const std::string CNC_FILE = "cnc.dat";

static const std::int32_t CNC_VERSION = 5;

#pragma pack(push)
#pragma pack(4)
struct MetaDataDefn
{
    std::int32_t cncVersion;
    std::int32_t toDriverBufferLength;
    std::int32_t toClientsBufferLength;
    std::int32_t counterMetadataBufferLength;
    std::int32_t counterValuesBufferLength;
    std::int64_t clientLivenessTimeout;
    std::int32_t errorLogBufferLength;
};
#pragma pack(pop)

static const size_t VERSION_AND_META_DATA_LENGTH = BitUtil::align(sizeof(MetaDataDefn), BitUtil::CACHE_LINE_LENGTH * 2);

inline static std::int32_t cncVersion(MemoryMappedFile::ptr_t cncFile)
{
    AtomicBuffer metaDataBuffer(cncFile->getMemoryPtr(), cncFile->getMemorySize());

    const MetaDataDefn& metaData = metaDataBuffer.overlayStruct<MetaDataDefn>(0);

    return metaData.cncVersion;
}

inline static AtomicBuffer createToDriverBuffer(MemoryMappedFile::ptr_t cncFile)
{
    AtomicBuffer metaDataBuffer(cncFile->getMemoryPtr(), cncFile->getMemorySize());

    const MetaDataDefn& metaData = metaDataBuffer.overlayStruct<MetaDataDefn>(0);

    return AtomicBuffer(cncFile->getMemoryPtr() + VERSION_AND_META_DATA_LENGTH, metaData.toDriverBufferLength);
}

inline static AtomicBuffer createToClientsBuffer(MemoryMappedFile::ptr_t cncFile)
{
    AtomicBuffer metaDataBuffer(cncFile->getMemoryPtr(), cncFile->getMemorySize());

    const MetaDataDefn& metaData = metaDataBuffer.overlayStruct<MetaDataDefn>(0);
    std::uint8_t* basePtr = cncFile->getMemoryPtr() + VERSION_AND_META_DATA_LENGTH + metaData.toDriverBufferLength;

    return AtomicBuffer(basePtr, metaData.toClientsBufferLength);
}

inline static AtomicBuffer createCounterMetadataBuffer(MemoryMappedFile::ptr_t cncFile)
{
    AtomicBuffer metaDataBuffer(cncFile->getMemoryPtr(), cncFile->getMemorySize());

    const MetaDataDefn& metaData = metaDataBuffer.overlayStruct<MetaDataDefn>(0);
    std::uint8_t* basePtr =
        cncFile->getMemoryPtr() +
        VERSION_AND_META_DATA_LENGTH +
        metaData.toDriverBufferLength +
        metaData.toClientsBufferLength;

    return AtomicBuffer(basePtr, metaData.counterMetadataBufferLength);
}

inline static AtomicBuffer createCounterValuesBuffer(MemoryMappedFile::ptr_t cncFile)
{
    AtomicBuffer metaDataBuffer(cncFile->getMemoryPtr(), cncFile->getMemorySize());

    const MetaDataDefn& metaData = metaDataBuffer.overlayStruct<MetaDataDefn>(0);
    std::uint8_t* basePtr =
        cncFile->getMemoryPtr() +
        VERSION_AND_META_DATA_LENGTH +
        metaData.toDriverBufferLength +
        metaData.toClientsBufferLength +
        metaData.counterMetadataBufferLength;

    return AtomicBuffer(basePtr, metaData.counterValuesBufferLength);
}

inline static AtomicBuffer createErrorLogBuffer(MemoryMappedFile::ptr_t cncFile)
{
    AtomicBuffer metaDataBuffer(cncFile->getMemoryPtr(), cncFile->getMemorySize());

    const MetaDataDefn& metaData = metaDataBuffer.overlayStruct<MetaDataDefn>(0);
    std::uint8_t* basePtr =
        cncFile->getMemoryPtr() +
            VERSION_AND_META_DATA_LENGTH +
            metaData.toDriverBufferLength +
            metaData.toClientsBufferLength +
            metaData.counterMetadataBufferLength +
            metaData.counterValuesBufferLength;

    return AtomicBuffer(basePtr, metaData.errorLogBufferLength);
}

inline static std::int64_t clientLivenessTimeout(MemoryMappedFile::ptr_t cncFile)
{
    AtomicBuffer metaDataBuffer(cncFile->getMemoryPtr(), cncFile->getMemorySize());

    const MetaDataDefn& metaData = metaDataBuffer.overlayStruct<MetaDataDefn>(0);

    return metaData.clientLivenessTimeout;
}

}

};

#endif