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
#include "InfraworldRuntime.h"
#include "PlatformTime.h"

#include "RpcClientWorker.h"
#include "GenUtils.h"

#include <grpc++/channel.h>
#include <grpc++/create_channel.h>


// Since GRPC actively uses winapi, we need to forbid windows macros
// (such as GetMessage, MemoryBarrier, etc.) in our remaining code.
// To do it, we 'wrap' all the C++ file's including ANY GRPC header files
// content into Allow/Hide WindowsPlatformTypes.
// We're unable to 'isolate' the WinAPI usage within a single C++ file thanks to Unity Build.
#if PLATFORM_WINDOWS
#include "AllowWindowsPlatformTypes.h"
#endif


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

//std::shared_ptr<grpc::Channel> RpcClientWorker::CreateChannel()
//{
//    UChannelCredentials* const ChannelCredentials = DispatcherParameters.ChannelCredentials;
//    UE_CLOG(!ChannelCredentials, LogTemp, Fatal, TEXT("Channel Credentials mustn't be null"));
//
//    const FString& ChannelAddess = DispatcherParameters.GetURI();
//    UE_LOG(LogTemp, Display, TEXT("The following Channel Credentials is used: %s. Connecting to: %s"), *(ChannelCredentials->GetName()), *ChannelAddess);
//
//    //std::shared_ptr<grpc::Channel> Channel = grpc::CreateChannel(TCHAR_TO_UTF8(*ChannelAddess), UChannelCredentials::GetGrpcCredentials(ChannelCredentials));
//    std::shared_ptr<grpc::Channel> Channel = grpc::CreateChannel("127.0.0.1:50051", grpc::InsecureChannelCredentials());
//
//    volatile bool b = WaitForConnection(3, Channel);
//    UE_LOG(LogTemp, Display, TEXT(""));
//    if (!b)
//    {
//        DispatchError(TEXT("Connection failure"));
//        return std::shared_ptr<grpc::Channel>(nullptr);
//    }
//    else
//    {
//        UE_LOG(LogTemp, Display, TEXT("Connection established!"));
//    }
//
//    return Channel;
//}

//bool RpcClientWorker::WaitUntilChannelIsReady(const std::shared_ptr<grpc::Channel>& Channel, std::chrono::system_clock::time_point Deadline)
//{
//    grpc_connectivity_state State = Channel->GetState(true);
//
//    while (State != GRPC_CHANNEL_READY)
//    {
//        if (!Channel->WaitForStateChange(State, Deadline))
//            return false;
//
//        State = Channel->GetState(true);
//    }
//
//    return true;
//}
//
//bool RpcClientWorker::WaitForConnection(float Seconds, const std::shared_ptr<grpc::Channel>& Channel)
//{
//    bool IsConnected = false;
//
//    const int64 Milliseconds = (int64)((double)Seconds * 1000.0);
//
//    std::chrono::system_clock::time_point start_tp = std::chrono::system_clock::now();
//    std::chrono::system_clock::time_point end_tp = std::chrono::system_clock::now() + std::chrono::milliseconds(Milliseconds);
//
//    std::chrono::system_clock::time_point current_tp = start_tp;
//
//    while (!IsConnected)
//    {
//        std::chrono::system_clock::time_point delta_tp = std::chrono::system_clock::now() + std::chrono::milliseconds(100);
//
//        if (current_tp < end_tp)
//            IsConnected = WaitUntilChannelIsReady(Channel, delta_tp);
//        else
//            break;
//
//        current_tp = std::chrono::system_clock::now();
//    }
//
//    return IsConnected;
//}

#if PLATFORM_WINDOWS
#include "HideWindowsPlatformTypes.h"
#endif
