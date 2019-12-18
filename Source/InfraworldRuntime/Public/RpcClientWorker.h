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

#include "Containers/Queue.h"
#include "ChannelCredentials.h"
#include "HAL/Runnable.h"
#include "InfraworldRuntime.h"
#include <memory>
#include <chrono>

#include "Templates/Atomic.h"

enum class ERpcWorkerState : uint8
{
	PendingInitialization,
	Initializing,
	Working,
	PendingShutdown,
	Shutdown
};

class FGenAsyncRequest;

/**
 * Base RPC Client Worker, it 'lives' in a separate thread and updates all conduits with responses.
 */
class INFRAWORLDRUNTIME_API RpcClientWorker : public FRunnable
{
public:
    RpcClientWorker();
	virtual ~RpcClientWorker();

    virtual uint32 Run() override;

    FORCEINLINE bool IsPendingStopped() const
    {
        return WorkerState.Load() == ERpcWorkerState::PendingShutdown;
    }

    FORCEINLINE void MarkPendingStopped()
    {
		UE_LOG(LogInfraworldRuntime, Log, TEXT("RpcClientWorker at [%p] Marking pending stopped"), this);
		const ERpcWorkerState PreviousWorkerState = WorkerState.Exchange(ERpcWorkerState::PendingShutdown);
		static const TSet<ERpcWorkerState> ExpectedWorkerStates = {
			ERpcWorkerState::PendingInitialization, ERpcWorkerState::Initializing, ERpcWorkerState::Working
		};
		ensureAlways(ExpectedWorkerStates.Contains(PreviousWorkerState));    
    }
	
    virtual bool HierarchicalInit() = 0;
	virtual void HierarchicalUpdate() = 0;

    void DispatchError(const FString& ErrorMessage);

//public:
    FString URI;
    UChannelCredentials* ChannelCredentials;

    TQueue<FRpcError>* ErrorMessageQueue;
	
protected:
	TAtomic<ERpcWorkerState> WorkerState;
};
