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

#ifndef INCLUDED_AERON_AERON__
#define INCLUDED_AERON_AERON__

#include <iostream>
#include <thread>
#include <random>

#include "util/Exceptions.h"
#include "util/MemoryMappedFile.h"
#include "concurrent/logbuffer/TermReader.h"
#include "concurrent/broadcast/CopyBroadcastReceiver.h"
#include "concurrent/SleepingIdleStrategy.h"
#include "concurrent/AgentRunner.h"

#include "ClientConductor.h"
#include "Publication.h"
#include "Subscription.h"
#include "Context.h"

/// Top namespace for Aeron C++ API
namespace aeron {

using namespace aeron::util;
using namespace aeron::concurrent;
using namespace aeron::concurrent::broadcast;

/**
 * @example BasicPublisher.cpp
 * An example of a basic publishing application
 *
 * @example BasicSubscriber.cpp
 * An example of a basic subscribing application
 */

/**
 * Aeron entry point for communicating to the Media Driver for creating {@link Publication}s and {@link Subscription}s.
 * Use a {@link Context} to configure the Aeron object.
 * <p>
 * A client application requires only one Aeron object per Media Driver.
 */
class Aeron
{
public:
    /**
     * Create an Aeron instance and connect to the media driver.
     * <p>
     * Threads required for interacting with the media driver are created and managed within the Aeron instance.
     *
     * @param context for configuration of the client.
     */
    Aeron(Context& context);
    virtual ~Aeron();

    /**
     * Create an Aeron instance and connect to the media driver.
     * <p>
     * Threads required for interacting with the media driver are created and managed within the Aeron instance.
     *
     * @param context for configuration of the client.
     * @return the new Aeron instance connected to the Media Driver.
     */
    inline static std::shared_ptr<Aeron> connect(Context& context)
    {
        return std::make_shared<Aeron>(context);
    }

    /**
     * Add a {@link Publication} for publishing messages to subscribers
     *
     * This function returns immediately and does not wait for the response from the media driver. The returned
     * registration id is to be used to determine the status of the command with the media driver.
     *
     * @param channel for receiving the messages known to the media layer.
     * @param streamId within the channel scope.
     * @return registration id for the publication
     */
    inline std::int64_t addPublication(const std::string& channel, std::int32_t streamId)
    {
        return m_conductor.addPublication(channel, streamId);
    }

    /**
     * Retrieve the Publication associated with the given registrationId.
     *
     * This method is non-blocking.
     *
     * The value returned is dependent on what has occurred with respect to the media driver:
     *
     * - If the registrationId is unknown, then a nullptr is returned.
     * - If the media driver has not answered the add command, then a nullptr is returned.
     * - If the media driver has successfully added the Publication then what is returned is the Publication.
     * - If the media driver has returned an error, this method will throw the error returned.
     *
     * @see Aeron::addPublication
     *
     * @param registrationId of the Publication returned by Aeron::addPublication
     * @return Publication associated with the registrationId
     */
    inline std::shared_ptr<Publication> findPublication(std::int64_t registrationId)
    {
        return m_conductor.findPublication(registrationId);
    }

    /**
     * Add a new {@link Subscription} for subscribing to messages from publishers.
     *
     * This function returns immediately and does not wait for the response from the media driver. The returned
     * registration id is to be used to determine the status of the command with the media driver.
     *
     * @param channel  for receiving the messages known to the media layer.
     * @param streamId within the channel scope.
     * @return registration id for the subscription
     */
    inline std::int64_t addSubscription(const std::string& channel, std::int32_t streamId)
    {
        return m_conductor.addSubscription(channel, streamId);
    }

    /**
     * Retrieve the Subscription associated with the given registrationId.
     *
     * This method is non-blocking.
     *
     * The value returned is dependent on what has occurred with respect to the media driver:
     *
     * - If the registrationId is unknown, then a nullptr is returned.
     * - If the media driver has not answered the add command, then a nullptr is returned.
     * - If the media driver has successfully added the Subscription then what is returned is the Subscription.
     * - If the media driver has returned an error, this method will throw the error returned.
     *
     * @see Aeron::addSubscription
     *
     * @param registrationId of the Subscription returned by Aeron::addSubscription
     * @return Subscription associated with the registrationId
     */
    inline std::shared_ptr<Subscription> findSubscription(std::int64_t registrationId)
    {
        return m_conductor.findSubscription(registrationId);
    }

private:
    std::random_device m_randomDevice;
    std::default_random_engine m_randomEngine;
    std::uniform_int_distribution<std::int32_t> m_sessionIdDistribution;

    Context& m_context;

    MemoryMappedFile::ptr_t m_cncBuffer;

    AtomicBuffer m_toDriverAtomicBuffer;
    AtomicBuffer m_toClientsAtomicBuffer;
    AtomicBuffer m_countersValueBuffer;

    ManyToOneRingBuffer m_toDriverRingBuffer;
    DriverProxy m_driverProxy;

    BroadcastReceiver m_toClientsBroadcastReceiver;
    CopyBroadcastReceiver m_toClientsCopyReceiver;

    ClientConductor m_conductor;
    SleepingIdleStrategy m_idleStrategy;
    AgentRunner<ClientConductor, SleepingIdleStrategy> m_conductorRunner;

    MemoryMappedFile::ptr_t mapCncFile(Context& context);
};

}

#endif