/*
 * Copyright 2018 Vizor Games LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.See the
 * License for the specific language governing permissions and limitations
 * under the License.
 */
#pragma once

#include "CoreMinimal.h"
#include "HAL/PlatformTLS.h"
#include "Containers/Queue.h"

/**
 * A conduit is a combination of two channel: The Request channel, and the Response channel, representing bidirectional queue.
 * A conduit is optimized to work efficiently and lock-free between two threads: the 'Request writer' thread and the
 *  'Response writer' thread.
 * One should call Acquire(Requests/Response)Producer() in the thread, which should produce Requests or Responses.
 */
template<class TRequest, class TResponse>
class TConduit
{
    FORCEINLINE uint32 ThreadID() const { return FPlatformTLS::GetCurrentThreadId(); }

public:
    TConduit() : RequestsProducerID(-1), ResponsesProducerID(-1)
    {
    }

    ~TConduit();

    /**
     * Should be called from a Request producer thread.
     * After that:
     *  - Only THIS thread can call Enqueue(Request),
     *  - Only THIS thread can call Dequeue(Response),
     *  - Calling IsEmpty() will tell whether the Request channel is empty.
     */
    void AcquireRequestsProducer()
    {
        RequestsProducerID = ThreadID();
    }

    /**
     * Should be called from a Response producer thread.
     * After that:
     *  - Only THIS thread can call Enqueue(Response),
     *  - Only THIS thread can call Dequeue(Request),
     *  - Calling IsEmpty() will tell whether the Response channel is empty.
     */
    void AcquireResponsesProducer()
    {
        ResponsesProducerID = ThreadID();
    }

// Enqueue:
    bool Enqueue(const TRequest& Item)
    {
        UE_CLOG(ThreadID() != RequestsProducerID, LogTemp, Fatal, TEXT("Can't call Enqueue(const TRequest&), invalid thread. Expected: %u, got: %u"), ResponsesProducerID, ThreadID());
        return Requests.Enqueue(Item);
    }

    bool Enqueue(const TResponse& Item)
    {
        UE_CLOG(ThreadID() != ResponsesProducerID, LogTemp, Fatal, TEXT("Can't call Enqueue(const TResponse&), invalid thread. Expected: %u, got: %u"), RequestsProducerID, ThreadID());
        return Responses.Enqueue(Item);
    }

    bool Enqueue(TRequest&& Item)
    {
        UE_CLOG(ThreadID() != RequestsProducerID, LogTemp, Fatal, TEXT("Can't call Enqueue(const TRequest&), invalid thread. Expected: %u, got: %u"), ResponsesProducerID, ThreadID());
        return Requests.Enqueue(Item);
    }

    bool Enqueue(TResponse&& Item)
    {
        UE_CLOG(ThreadID() != ResponsesProducerID, LogTemp, Fatal, TEXT("Can't call Enqueue(const TResponse&), invalid thread. Expected: %u, got: %u"), RequestsProducerID, ThreadID());
        return Responses.Enqueue(Item);
    }

// Dequeue
    bool Dequeue(TRequest& OutItem)
    {
        UE_CLOG(ThreadID() != ResponsesProducerID, LogTemp, Fatal, TEXT("Can't call Dequeue(TRequest& OutItem), invalid thread. Expected: %u, got: %u"), ResponsesProducerID, ThreadID());
        return Requests.Dequeue(OutItem);
    }

    bool Dequeue(TResponse& OutItem)
    {
        UE_CLOG(ThreadID() != RequestsProducerID, LogTemp, Fatal, TEXT("Can't call Dequeue(TResponse& OutItem), invalid thread. Expected: %u, got: %u"), RequestsProducerID, ThreadID());
        return Responses.Dequeue(OutItem);
    }

// Is Empty?
    bool IsEmpty() const
    {
        const uint32 Id = ThreadID();

        if (Id == RequestsProducerID)
            return Responses.IsEmpty();
        else if (Id == ResponsesProducerID)
            return Requests.IsEmpty();
        else
        {
            UE_LOG(LogTemp, Fatal, TEXT("Can't call IsEmpty(), from an unknown thread: %d, RequestsProducerID: %u, ResponsesProducerID: %u"), Id, RequestsProducerID, ResponsesProducerID);
            return false;
        }
    }

private:
    TQueue<TRequest> Requests;
    TQueue<TResponse> Responses;

    volatile uint32 RequestsProducerID;
    volatile uint32 ResponsesProducerID;
};

template<class TRequest, class TResponse>
TConduit<TRequest, TResponse>::~TConduit()
{

}
