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
#include "RpcClientWorker.h"

#include "InfraworldRuntime.h"

#include "HAL/PlatformTime.h"
#include "GenUtils.h"

#include "GrpcIncludesBegin.h"

#include <grpc++/channel.h>
#include <grpc++/create_channel.h>

#include "GrpcIncludesEnd.h"


// ========= RpcClientWorker implementation ========

RpcClientWorker::RpcClientWorker() : bPendingStopped(false)
{
}

RpcClientWorker::~RpcClientWorker()
{
}

uint32 RpcClientWorker::Run()
{
    // If channel has not been created - we set bPendingStopped = true to StopBackground.
    if (HierarchicalInit())
        bPendingStopped = false;
    else
        return 1;

    // Update until not pending stopped
    while (!bPendingStopped)
    {
        UE_LOG(LogTemp, Verbose, TEXT("Updating via HierarchicalUpdate()"));

        HierarchicalUpdate();
        FPlatformProcess::Sleep(0.1f);
    }

    return 0;
}

void RpcClientWorker::DispatchError(const FString& ErrorMessage)
{
    UE_CLOG(!ErrorMessageQueue, LogTemp, Fatal, TEXT("Can not dispatch an error message, because ErrorMessageQueue is null"));

    FRpcError Error;
    Error.ErrorMessage = ErrorMessage;

    ErrorMessageQueue->Enqueue(Error);
}

#if PLATFORM_WINDOWS
#include "Windows/HideWindowsPlatformTypes.h"
#endif
