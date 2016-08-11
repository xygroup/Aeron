/*
 * Copyright 2015 - 2016 Real Logic Ltd.
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

#ifndef INCLUDED_AERON_DRIVER_MEDIA_SENDCHANNELENDPOINT__
#define INCLUDED_AERON_DRIVER_MEDIA_SENDCHANNELENDPOINT__

#include "aeron/protocol/DataHeaderFlyweight.h"
#include "aeron/protocol/StatusMessageFlyweight.h"

#include "UdpChannelTransport.h"

namespace aeron { namespace driver { namespace media {

using namespace aeron::protocol;

class SendChannelEndpoint : public UdpChannelTransport
{
public:
    inline SendChannelEndpoint(std::unique_ptr<UdpChannel>&& channel)
        : UdpChannelTransport(channel, &channel->remoteControl(), &channel->localControl(), &channel->remoteData()),
          m_dataHeaderFlyweight(receiveBuffer(), 0),
          m_smFlyweight(receiveBuffer(), 0)
    {
    }

private:
    DataHeaderFlyweight m_dataHeaderFlyweight;
    StatusMessageFlyweight m_smFlyweight;
};

}}}



#endif //INCLUDED_AERON_DRIVER_MEDIA_SENDCHANNELENDPOINT__
