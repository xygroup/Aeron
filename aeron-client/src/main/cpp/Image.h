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

#ifndef AERON_IMAGE_H
#define AERON_IMAGE_H

#include <algorithm>
#include <atomic>

#include "concurrent/AtomicBuffer.h"
#include "concurrent/logbuffer/LogBufferDescriptor.h"
#include "concurrent/logbuffer/Header.h"
#include "concurrent/logbuffer/TermReader.h"
#include "concurrent/logbuffer/TermBlockScanner.h"
#include "concurrent/status/UnsafeBufferPosition.h"

#include "LogBuffers.h"

namespace aeron {

using namespace aeron::concurrent;
using namespace aeron::concurrent::logbuffer;
using namespace aeron::concurrent::status;

static UnsafeBufferPosition NULL_POSITION;

static const int IMAGE_CLOSED = -1;

enum class ControlledPollAction : int
{
    /**
     * Abort the current polling operation and do not advance the position for this fragment.
     */
    ABORT = 1,

    /**
     * Break from the current polling operation and commit the position as of the end of the current fragment
     * being handled.
     */
    BREAK,

    /**
     * Continue processing but commit the position as of the end of the current fragment so that
     * flow control is applied to this point.
     */
    COMMIT,

    /**
     * Continue processing taking the same approach as the in fragment_handler_t.
     */
    CONTINUE
};

/**
 * Callback for handling fragments of data being read from a log.
 *
 * @param buffer containing the data.
 * @param offset at which the data begins.
 * @param length of the data in bytes.
 * @param header representing the meta data for the data.
 * @return The action to be taken with regard to the stream position after the callback.
 */
typedef std::function<ControlledPollAction(
    concurrent::AtomicBuffer &buffer,
    util::index_t offset,
    util::index_t length,
    Header &header)> controlled_poll_fragment_handler_t;

/**
 * Represents a replicated publication {@link Image} from a publisher to a {@link Subscription}.
 * Each {@link Image} identifies a source publisher by session id.
 *
 * Is an overlay on the LogBuffers and Position. So, can be effectively copied and moved.
 */
class Image
{
public:
    typedef Image this_t;

    Image() :
        m_header(0, 0),
        m_subscriberPosition(NULL_POSITION)
    {
    }

    /**
     * Construct a new image over a log to represent a stream of messages from a {@link Publication}.
     *
     * @param sessionId          of the stream of messages.
     * @param initialPosition    at which the subscriber is joining the stream.
     * @param subscriberPosition for indicating the position of the subscriber in the stream.
     * @param logBuffers         containing the stream of messages.
     * @param correlationId      of the image with the media driver.
     * @param subscriptionRegistrationId of the Subscription.
     * @param exceptionHandler   to call if an exception is encountered on polling.
     */
    Image(
        std::int32_t sessionId,
        std::int64_t correlationId,
        std::int64_t subscriptionRegistrationId,
        const std::string& sourceIdentity,
        UnsafeBufferPosition& subscriberPosition,
        std::shared_ptr<LogBuffers> logBuffers,
        const exception_handler_t& exceptionHandler) :
        m_header(
            LogBufferDescriptor::initialTermId(logBuffers->atomicBuffer(LogBufferDescriptor::LOG_META_DATA_SECTION_INDEX)),
            logBuffers->atomicBuffer(0).capacity()),
        m_subscriberPosition(subscriberPosition),
        m_logBuffers(logBuffers),
        m_sourceIdentity(sourceIdentity),
        m_isClosed(false),
        m_exceptionHandler(exceptionHandler),
        m_correlationId(correlationId),
        m_subscriptionRegistrationId(subscriptionRegistrationId),
        m_sessionId(sessionId)
    {
        for (int i = 0; i < LogBufferDescriptor::PARTITION_COUNT; i++)
        {
            m_termBuffers[i] = logBuffers->atomicBuffer(i);
        }

        const util::index_t capacity = m_termBuffers[0].capacity();

        m_termLengthMask = capacity - 1;
        m_positionBitsToShift = BitUtil::numberOfTrailingZeroes(capacity);
    }

    Image(const Image& image) :
        m_header(image.m_header),
        m_subscriberPosition(image.m_subscriberPosition),
        m_sourceIdentity(image.m_sourceIdentity),
        m_isClosed(image.isClosed()),
        m_exceptionHandler(image.m_exceptionHandler)
    {
        for (int i = 0; i < LogBufferDescriptor::PARTITION_COUNT; i++)
        {
            m_termBuffers[i].wrap(image.m_termBuffers[i]);
        }

        m_subscriberPosition.wrap(image.m_subscriberPosition);
        m_logBuffers = image.m_logBuffers;
        m_correlationId = image.m_correlationId;
        m_subscriptionRegistrationId = image.m_subscriptionRegistrationId;
        m_sessionId = image.m_sessionId;
        m_termLengthMask = image.m_termLengthMask;
        m_positionBitsToShift = image.m_positionBitsToShift;
    }

    Image& operator=(Image& image)
    {
        for (int i = 0; i < LogBufferDescriptor::PARTITION_COUNT; i++)
        {
            m_termBuffers[i].wrap(image.m_termBuffers[i]);
        }

        m_header = image.m_header;
        m_subscriberPosition.wrap(image.m_subscriberPosition);
        m_logBuffers = image.m_logBuffers;
        m_sourceIdentity = image.m_sourceIdentity;
        m_isClosed = image.isClosed();
        m_exceptionHandler = image.m_exceptionHandler;
        m_correlationId = image.m_correlationId;
        m_subscriptionRegistrationId = image.m_subscriptionRegistrationId;
        m_sessionId = image.m_sessionId;
        m_termLengthMask = image.m_termLengthMask;
        m_positionBitsToShift = image.m_positionBitsToShift;
        return *this;
    }

    Image& operator=(Image&& image)
    {
        for (int i = 0; i < LogBufferDescriptor::PARTITION_COUNT; i++)
        {
            m_termBuffers[i].wrap(image.m_termBuffers[i]);
        }

        m_header = image.m_header;
        m_subscriberPosition.wrap(image.m_subscriberPosition);
        m_logBuffers = std::move(image.m_logBuffers);
        m_sourceIdentity = std::move(image.m_sourceIdentity);
        m_isClosed = image.isClosed();
        m_exceptionHandler = image.m_exceptionHandler;
        m_correlationId = image.m_correlationId;
        m_subscriptionRegistrationId = image.m_subscriptionRegistrationId;
        m_sessionId = image.m_sessionId;
        m_termLengthMask = image.m_termLengthMask;
        m_positionBitsToShift = image.m_positionBitsToShift;
        return *this;
    }

    virtual ~Image() = default;

    /**
     * Get the length in bytes for each term partition in the log buffer.
     *
     * @return the length in bytes for each term partition in the log buffer.
     */
    inline std::int32_t termBufferLength() const
    {
        return m_termBuffers[0].capacity();
    }

    /**
     * The sessionId for the steam of messages.
     *
     * @return the sessionId for the steam of messages.
     */
    inline std::int32_t sessionId() const
    {
        return m_sessionId;
    }

    /**
     * The correlationId for identification of the image with the media driver.
     *
     * @return the correlationId for identification of the image with the media driver.
     */
    inline std::int64_t correlationId() const
    {
        return m_correlationId;
    }

    /**
     * The registrationId for the Subscription of the Image.
     *
     * @return the registrationId for the Subscription of the Image.
     */
    inline std::int64_t subscriptionRegistrationId() const
    {
        return m_subscriptionRegistrationId;
    }

    /**
     * The initial term at which the stream started for this session.
     *
     * @return the initial term id.
     */
    inline std::int32_t initialTermId() const
    {
        return m_header.initialTermId();
    }

    /**
     * The source identity of the sending publisher as an abstract concept appropriate for the media.
     *
     * @return source identity of the sending publisher as an abstract concept appropriate for the media.
     */
    inline std::string sourceIdentity() const
    {
        return m_sourceIdentity;
    }

    /**
     * Has this object been closed and should no longer be used?
     *
     * @return true if it has been closed otherwise false.
     */
    inline bool isClosed() const
    {
        return std::atomic_load_explicit(&m_isClosed, std::memory_order_relaxed);
    }

    /**
     * The position this Image has been consumed to by the subscriber.
     *
     * @return the position this Image has been consumed to by the subscriber or CLOSED if closed
     */
    inline std::int64_t position()
    {
        std::int64_t result = IMAGE_CLOSED;

        if (!isClosed())
        {
            result = m_subscriberPosition.get();
        }

        return result;
    }

    /**
     * Poll for new messages in a stream. If new messages are found beyond the last consumed position then they
     * will be delivered via the fragment_handler_t up to a limited number of fragments as specified.
     *
     * @param fragmentHandler to which messages are delivered.
     * @param fragmentLimit   for the number of fragments to be consumed during one polling operation.
     * @return the number of fragments that have been consumed.
     *
     * @see fragment_handler_t
     */
    int poll(const fragment_handler_t& fragmentHandler, int fragmentLimit)
    {
        int result = IMAGE_CLOSED;

        if (!isClosed())
        {
            const std::int64_t position = m_subscriberPosition.get();
            const std::int32_t termOffset = (std::int32_t) position & m_termLengthMask;
            AtomicBuffer &termBuffer = m_termBuffers[LogBufferDescriptor::indexByPosition(position,
                m_positionBitsToShift)];
            TermReader::ReadOutcome readOutcome;

            TermReader::read(readOutcome, termBuffer, termOffset, fragmentHandler, fragmentLimit, m_header, m_exceptionHandler);

            const std::int64_t newPosition = position + (readOutcome.offset - termOffset);
            if (newPosition > position)
            {
                m_subscriberPosition.setOrdered(newPosition);
            }

            result = readOutcome.fragmentsRead;
        }

        return result;
    }

    /**
     * Poll for new messages in a stream. If new messages are found beyond the last consumed position then they
     * will be delivered to the controlled_poll_fragment_handler_t up to a limited number of fragments as specified.
     *
     * To assemble messages that span multiple fragments then use ControlledFragmentAssembler.
     *
     * @param fragmentHandler to which message fragments are delivered.
     * @param fragmentLimit   for the number of fragments to be consumed during one polling operation.
     * @return the number of fragments that have been consumed.
     *
     * @see controlled_poll_fragment_handler_t
     */
    int controlledPoll(const controlled_poll_fragment_handler_t& fragmentHandler, int fragmentLimit)
    {
        int result = IMAGE_CLOSED;

        if (!isClosed())
        {
            std::int64_t position = m_subscriberPosition.get();
            std::int32_t termOffset = (std::int32_t) position & m_termLengthMask;
            AtomicBuffer &termBuffer = m_termBuffers[LogBufferDescriptor::indexByPosition(position,
                m_positionBitsToShift)];
            int fragmentsRead = 0;
            std::int32_t offset = termOffset;

            try
            {
                const util::index_t capacity = termBuffer.capacity();

                do
                {
                    const std::int32_t length = FrameDescriptor::frameLengthVolatile(termBuffer, offset);
                    if (length <= 0)
                    {
                        break;
                    }

                    const std::int32_t frameOffset = offset;
                    const std::int32_t alignedLength = util::BitUtil::align(length, FrameDescriptor::FRAME_ALIGNMENT);
                    offset += alignedLength;

                    if (!FrameDescriptor::isPaddingFrame(termBuffer, frameOffset))
                    {
                        m_header.buffer(termBuffer);
                        m_header.offset(frameOffset);

                        const ControlledPollAction action =
                            fragmentHandler(
                                termBuffer,
                                frameOffset + DataFrameHeader::LENGTH,
                                length - DataFrameHeader::LENGTH,
                                m_header);

                        ++fragmentsRead;

                        if (ControlledPollAction::BREAK == action)
                        {
                            break;
                        }
                        else if (ControlledPollAction::ABORT == action)
                        {
                            --fragmentsRead;
                            offset = frameOffset;
                            break;
                        }
                        else if (ControlledPollAction::COMMIT == action)
                        {
                            position += alignedLength;
                            termOffset = offset;
                            m_subscriberPosition.setOrdered(position);
                        }
                    }
                }
                while (fragmentsRead < fragmentLimit && offset < capacity);
            }
            catch (std::exception ex)
            {
                m_exceptionHandler(ex);
            }

            const std::int64_t newPosition = position + (offset - termOffset);
            if (newPosition > position)
            {
                m_subscriberPosition.setOrdered(newPosition);
            }

            result = fragmentsRead;
        }

        return result;
    }

    /**
     * Poll for new messages in a stream. If new messages are found beyond the last consumed position then they
     * will be delivered via the block_handler_t up to a limited number of bytes.
     *
     * @param blockHandler     to which block is delivered.
     * @param blockLengthLimit up to which a block may be in length.
     * @return the number of bytes that have been consumed.
     *
     * @see block_handler_t
     */
    int blockPoll(const block_handler_t& blockHandler, int blockLengthLimit)
    {
        int result = IMAGE_CLOSED;

        if (!isClosed())
        {
            const std::int64_t position = m_subscriberPosition.get();
            const std::int32_t termOffset = (std::int32_t) position & m_termLengthMask;
            AtomicBuffer &termBuffer = m_termBuffers[LogBufferDescriptor::indexByPosition(position,
                m_positionBitsToShift)];
            const std::int32_t limit = std::min(termOffset + blockLengthLimit, termBuffer.capacity());

            const std::int32_t resultingOffset = TermBlockScanner::scan(termBuffer, termOffset, limit);

            const std::int32_t bytesConsumed = resultingOffset - termOffset;

            if (resultingOffset > termOffset)
            {
                try
                {
                    const std::int32_t termId = termBuffer.getInt32(termOffset + DataFrameHeader::TERM_ID_FIELD_OFFSET);

                    blockHandler(termBuffer, termOffset, bytesConsumed, m_sessionId, termId);
                }
                catch (std::exception ex)
                {
                    m_exceptionHandler(ex);
                }

                m_subscriberPosition.setOrdered(position + bytesConsumed);
            }

            result = bytesConsumed;
        }

        return result;
    }

    // TODO: filePoll() with fd/HANDLE (or MemoryMappedFile) ptr access

    std::shared_ptr<LogBuffers> logBuffers()
    {
        return m_logBuffers;
    }

    /// @cond HIDDEN_SYMBOLS
    inline void close()
    {
        std::atomic_store_explicit(&m_isClosed, true, std::memory_order_relaxed);
    }
    /// @endcond

private:
    AtomicBuffer m_termBuffers[LogBufferDescriptor::PARTITION_COUNT];
    Header m_header;
    Position<UnsafeBufferPosition> m_subscriberPosition;
    std::shared_ptr<LogBuffers> m_logBuffers;
    std::string m_sourceIdentity;
    std::atomic<bool> m_isClosed;
    exception_handler_t m_exceptionHandler;

    std::int64_t m_correlationId;
    std::int64_t m_subscriptionRegistrationId;
    std::int32_t m_sessionId;
    std::int32_t m_termLengthMask;
    std::int32_t m_positionBitsToShift;
};

}

#endif //AERON_IMAGE_H
